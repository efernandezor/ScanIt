
#include <LiquidCrystal_I2C.h>  // RH - use this library for LCD displays with I2C backpack
#include <Stepper.h>
#include <SoftwareSerial.h>

const int SW_pin = 8; // digital pin connected to switch output
const int X_pin = A0; // analog pin connected to X output
const int Y_pin = A1; // analog pin connected to Y output

int bluetoothTx = 2;  // TX-O pin of bluetooth mate, Arduino D2
int bluetoothRx = 3;  // RX-I pin of bluetooth mate, Arduino D3

int MenuNr = 0;   // Menu number
int PhotoNr = 2;  // The amount of photos that have to be taken
bool Flag1 = 0;   // This flag is only active during 1 program cycle (prevents constantly adding/subtracting 1 to the menu number when the joystick is pushed to the side) 
bool Flag2 = 0;   // This flag is only active during 1 program cycle (prevents constantly adding/subtracting 2 to the photo number when the joystick is pushed up or down)
bool Flag3 = 0;   // This flag is only active during 1 program cycle (prevents constantly adding/subtracting 1 to the RPM when the joystick is pushed up or down)
bool Flag4 = 0;   // This flag is only active during 1 program cycle (prevents constantly adding/subtracting 1 to the turn number when the joystick is pushed to the side)
bool Flag5 = 0;   // This flag is only active during 1 program cycle (prevents constantly adding/subtracting 1 to the RPM when the joystick is pushed up or down)
bool Flag6 = 0;   // This flag is only active during 1 program cycle to clear the lcd
int SwMenu = 0;   // Switch menu (Sub menu's in the main menu's)
bool BtnFlag = 0; // This flag is only active during 1 program cycle (prevents constantly adding of 1 to SwMenu when button is pressed)

// RH - added variables for fast change & cancel modes
int FastChng = 0;  // indicates fast change value mode.  0 = off, 1 = delay mode, 2 = fast changing mode
const unsigned long FastDelay = 1000;  // delay mode time (before values change fast)
const unsigned long ShortInt = 100;  // short fast change interval
const unsigned long LongInt = 300;  // long fast change interval
const unsigned long BtnDelay = 2000;  // delay for button press to cancel operations.  Note this is an approximate delay, since stepper motor
                                     // suspends all program execution until motor finishes its move
unsigned long SetTime = 0; // time value for fast change & button cancel modes.  Used to calculate time intervals
bool BtnCancelFlag = 0; // This flag is used to detect when button is pressed for canceling operations
bool MaxSwMenu = 0;  // This flag is used for detecting when the maximum SwMenu is reached
bool CinCancelFlag = 0;  // This flag is used to trigger cinematic cancel.  1 = cancel cinematic operation
int StepPoll = 480;  // number of motor steps to poll for cinematic cancel (at 15 rpm)
int Cntr = 0;  // step counter for cinematic motor cancel
// RH - end of added variables

const int stepsPerRevolution = 2048;  // change this to fit the number of steps per revolution
int FullRev = 14336;                  // 1 full revolution of the big gear -> Small-Big gear ratio is 7:1
int rolePerMinute = 15;               // Adjustable range of 28BYJ-48 stepper is 0~17 rpm
int PhotoTaken = 0;                   // Amount of photo's that have been taken
int StepPerPhoto;                     // Amount of steps per photo (calculated -> see MenuNr 0/SwMenu 2)
int TurnNr = 1;                       // Amount of turns
int CurrentTurn = 0;                  // Stores current turn number
int Steps = 0;                        // Amount of individual steps the stepper motor has to turn

LiquidCrystal_I2C lcd(0x27, 16, 2);  // RH - use this for I2C LCD.  Assumes default address of 0x27
Stepper myStepper(stepsPerRevolution, 9, 11, 10, 12);  // Use these pins for the stepper motor
SoftwareSerial bluetooth(bluetoothTx, bluetoothRx);


void setup() {
  
  bluetooth.begin(115200);  // The Bluetooth Mate defaults to 115200bps
  delay(100);

  bluetooth.print("$$$");  // Enter mode configuration bluetooth
  delay(100);
  
  bluetooth.println("C"); //Especific mode keyboard
  delay(100);
  
  bluetooth.println("U,9600,N"); // Set baudrate 9600bps
  delay(100);
  
  bluetooth.begin(9600);
  
  lcd.init();                         // RH - I2C LCD init command
  lcd.backlight();                    // RH - I2C LCD turn backlight on
  pinMode(SW_pin, INPUT);             //Set pushbutton as input
  digitalWrite(SW_pin, HIGH);         //Set SW_pin High
  
  myStepper.setSpeed(rolePerMinute);  //Set RPM of steppermotor

  lcd.setCursor(4, 0);                //Startup screen start
  lcd.print("Welcome!");              //      """""      //
  lcd.setCursor(1, 1);                //      """""      //
  lcd.print("Software: V1.6");        //      """""      //
  delay(3000);                        //      """""      //
  lcd.clear();                        //      """""      //
  lcd.setCursor(0, 0);                //      """""      //
  lcd.print("Designed by");           //      """""      //
  lcd.setCursor(0, 1);                //      """""      //
  lcd.print("Erik,Alejandro,Adrian");         //      """""      //
  delay(2000);                        //      """""      //
  lcd.clear();                        //Startup screen end
}

void loop() {

  int XValue = analogRead(X_pin);     // Read the analog value from The X-axis from the joystick
  int YValue = analogRead(Y_pin);     // Read the analog value from The Y-axis from the joystick
  int SwValue = digitalRead(SW_pin);  // Read the digital value from The Button from the joystick

  if (MenuNr < 0){  //This sets the min number of menu's
    MenuNr = 0;
  }
  else if ( MenuNr > 2){  //This sets the max numbers of menu's
    MenuNr = 2;
  }
  
  if (XValue < 400 && Flag1 == 0 && SwMenu == 0){  //if the joystick is pushed to the right side and flag1 is 0 then 1 will be added to the menu number (purpose of the flag -> see comment Flags above)
    MenuNr = MenuNr + 1; 
    Flag1 = 1;
    lcd.clear();
  }

  if (XValue > 600 && Flag1 == 0 && SwMenu == 0){  //if the joystick is pushed to the left side and flag1 is 0 then 1 will be subtracted from the menu number (purpose of the flag -> see comment Flags above)
    MenuNr = MenuNr - 1;
    Flag1 = 1;
    lcd.clear();
  }
  
  if (XValue > 399 && XValue < 599 && Flag1 == 1){  //if joystick is at neutral possition, flag1 is set to 0 (purpose of the flag -> see comment Flags above)
    Flag1 = 0;
  }


  if (SwValue == 0 && BtnFlag == 0 && MaxSwMenu == 0){  //if the button is pressed and the flag is 0 -> add 1 to menu 
    SwMenu = SwMenu + 1;
    BtnFlag = 1;
    BtnCancelFlag = 0;
    lcd.clear();
  }

  if (SwValue == 1 && BtnFlag == 1){  //if the button is not pressed and the flag is 0 -> Reset the flag (purpose of the flag -> see comment Flags above)
    BtnFlag = 0;
  }
  


  switch (MenuNr){
    case 0: //***********************************************Menu0***********************************************//
            if (SwMenu == 0){ //Menu 0 selected
              lcd.setCursor(3, 0);
              lcd.print("Escaner");
              lcd.setCursor(14,0);
              lcd.print("->");
            }
          
            if (SwMenu == 1){ //entered menu 0
              lcd.setCursor(0, 0);
              lcd.print("Nr of photo's");
              lcd.setCursor(7, 1);
              lcd.print(PhotoNr);
        
              if (FastChng == 0) {  // RH - if not in fast change mode, update set time
                SetTime = millis();
              }
              
              if (YValue < 400 && Flag2 == 0){ //joystick up -> Add 2 to number of photo's
                PhotoNr = PhotoNr + 2;
                Flag2 = 1;
                FastChng = 1;
                lcd.clear();
              }
              if (YValue > 600 && Flag2 == 0){ //joystick down -> Subtract 2 from number of photo's
                PhotoNr = PhotoNr - 2;
                Flag2 = 1;
                FastChng = 1;
                lcd.clear();
              }
              if (YValue > 399 && YValue < 599 && Flag2 == 1){  //if the Y-axis is back at it's neutral possition -> Flag3 = 0 -> Prevents constant adding or subtracting of 2
                Flag2 = 0;
                FastChng = 0;
              }
        
              if (YValue < 400 && FastChng == 1) {  // RH - if joystick up is held for > the fast delay time, then enter fast change mode
                if ((millis() - SetTime) > FastDelay) {
                  FastChng = 2;
                  SetTime = millis();  // update the set time
                }
              }
        
              if (YValue > 600 && FastChng == 1) {  // RH - if joystick down is held for > the fast delay time, then enter fast change mode
                if ((millis() - SetTime) > FastDelay) {
                  FastChng = 2;
                  SetTime = millis();  // update the set time
                }
              }
        
              if (YValue < 400 && FastChng == 2) {  // RH - we are in fast change mode, so we want to update the values quickly, proportional between
                if ((millis() - SetTime) > (LongInt - (400 - YValue) * (LongInt - ShortInt) / 400)) {  // ShortInt and LongInt based on joystick deflection
                  PhotoNr = PhotoNr + 2;
                  SetTime = millis();
                  lcd.clear();
                }
              }
        
              if (YValue > 600 && FastChng == 2) {  // RH - same for joystick down in fast change mode
                if ((millis() - SetTime) > (LongInt - (YValue - 600) * (LongInt - ShortInt) / 400)) {
                  PhotoNr = PhotoNr - 2;
                  SetTime = millis();
                  lcd.clear();
                }
              }
        
              if (PhotoNr < 2){    //Min allowable Nr of photo's
                PhotoNr = 2;
              }
              if (PhotoNr > 200){  //Max allowable Nr of photo's
                PhotoNr = 200;
              }
            }
        
            if (SwMenu == 2){ //Program started
        
              MaxSwMenu = 1;
              
              lcd.setCursor(0, 0);
              lcd.print("Program started");
              lcd.setCursor(0, 1);
              lcd.print("Photo Nr: ");
              lcd.print(PhotoTaken);
              
              StepPerPhoto = FullRev / PhotoNr;  //Calculate amount of steps per photo
        
              if (PhotoTaken < PhotoNr){          
              myStepper.setSpeed(rolePerMinute);  //Set motor speed
              myStepper.step(StepPerPhoto);       //move the calculated amount of steps
              PhotoTaken = PhotoTaken + 1;        //Add 1 to photos taken
              lcd.setCursor(0, 1);
              lcd.print("Photo Nr: ");            //Taking photo's
              lcd.print(PhotoTaken);
              delay(100);
              bluetooth.println();                //Take a photo simulating press key enter
              delay(10000);
              }
        
              if (PhotoTaken == PhotoNr){  //If the amount of photos taken is equal to the amount of photos that have to be taken -> Program finished
                lcd.setCursor(0, 0);
                lcd.print("Program finished");
                delay(3000);
                lcd.clear();  //Rest parameters
                PhotoTaken = 0;
                PhotoNr = 2;
                SwMenu = 0;
                MaxSwMenu = 0;
                Steps = 0;
              }
        
              // RH NOTE - cancelling works but delay is longer than expected since stepper motor movement is blocking
              if (SwValue == 0 && BtnCancelFlag == 0) {  // if button is pressed, start timing for cancel
                BtnCancelFlag = 1;
                SetTime = millis();
              }
        
              if (SwValue == 1 && BtnCancelFlag == 1) {  // RH - if button released before BtnDelay, then reset flag
                BtnCancelFlag = 0;
              }
        
              if (SwValue == 0 && BtnCancelFlag == 1 && ((millis() - SetTime) > BtnDelay)) {  // RH - button has been held for > BtnDelay, so cancel operation
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Program cancel");
                delay(3000);
                lcd.clear();
                PhotoTaken = 0;
                PhotoNr = 2;
                SwMenu = 0;
                MaxSwMenu = 0;
                Steps = 0;
                BtnCancelFlag = 0;
              }
            }   
          break;
          
    case 1:  //***********************************************Menu1***********************************************//
            if (SwMenu == 0){  //Menu1 selected
                lcd.setCursor(0,0);
                lcd.print("<-");
                lcd.setCursor(3, 0);
                lcd.print("Cinematica");
                lcd.setCursor(14,0);
                lcd.print("->");
                CinCancelFlag = 0;
                Cntr = 0;
              }
          
              if (SwMenu == 1){  //Entered menu1 - sub menu1
                lcd.setCursor(0, 0);
                lcd.print("motor speed");
                lcd.setCursor(7, 1);
                lcd.print(rolePerMinute);
          
                if (FastChng == 0) {  // RH - if not in fast change mode, update set time
                  SetTime = millis();
                }
                
                if (YValue < 400 && Flag3 == 0){ // joystick up -> Add 1 RPM
                  rolePerMinute = rolePerMinute + 1;
                  Flag3 = 1;
                  FastChng = 1;
                  lcd.clear();
                }
                if (YValue > 600 && Flag3 == 0){ // joystick down -> Subtract 1 RPM
                  rolePerMinute = rolePerMinute - 1;
                  Flag3 = 1;
                  FastChng = 1;
                  lcd.clear();
                }
                if (YValue > 399 && YValue < 599 && Flag3 == 1){  //if the Y-axis is back at it's neutral possition -> Flag3 = 0 -> Prevents constant adding or subtracting of 1
                  Flag3 = 0;
                  FastChng = 0;
                }
          
                if (YValue < 400 && FastChng == 1) {  // RH - if joystick up is held for > the fast delay time, then enter fast change mode
                  if ((millis() - SetTime) > FastDelay) {
                    FastChng = 2;
                    SetTime = millis();  // update the set time
                  }
                }
          
                if (YValue > 600 && FastChng == 1) {  // RH - if joystick down is held for > the fast delay time, then enter fast change mode
                  if ((millis() - SetTime) > FastDelay) {
                    FastChng = 2;
                    SetTime = millis();  // update the set time
                  }
                }
          
                if (YValue < 400 && FastChng == 2) {  // RH - we are in fast change mode, so we want to update the values at the LongInt interval
                  if ((millis() - SetTime) > LongInt) {  // (using fixed LongInt since not that many values to cycle through)
                    rolePerMinute = rolePerMinute + 1;
                    SetTime = millis();
                    lcd.clear();
                  }
                }
          
                if (YValue > 600 && FastChng == 2) {  // RH - same for joystick down in fast change mode
                  if ((millis() - SetTime) > LongInt) {
                    rolePerMinute = rolePerMinute - 1;
                    SetTime = millis();
                    lcd.clear();
                  }
                }
                
                if (rolePerMinute < 1){  //Min allowable RPM
                  rolePerMinute = 1;
                }
                if (rolePerMinute > 17){  //Max allowable RPM
                  rolePerMinute = 17;
                }
              }
          
              if (SwMenu == 2){  //Entered menu1 - sub menu2
                lcd.setCursor(0, 0);
                lcd.print("Nr of turns");
                lcd.setCursor(7, 1);
                lcd.print(TurnNr);
          
                if (FastChng == 0) {  // RH - if not in fast change mode, update set time
                  SetTime = millis();
                }
                
                if (YValue < 400 && Flag4 == 0){ // joystick up -> Add 1 turn
                  TurnNr = TurnNr + 1;
                  Flag4 = 1;
                  FastChng = 1;
                  lcd.clear();
                }
                if (YValue > 600 && Flag4 == 0){ // joystick down -> Subtract 1 turn
                  TurnNr = TurnNr - 1;
                  Flag4 = 1;
                  FastChng = 1;
                  lcd.clear();
                }
                if (YValue > 399 && YValue < 599 && Flag4 == 1){  //if the Y-axis is back at it's neutral possition -> Flag3 = 0 -> Prevents constant adding or subtracting of 1
                  Flag4 = 0;
                  FastChng = 0;
                }
          
                if (YValue < 400 && FastChng == 1) {  // RH - if joystick up is held for > the fast delay time, then enter fast change mode
                  if ((millis() - SetTime) > FastDelay) {
                    FastChng = 2;
                    SetTime = millis();  // update the set time
                  }
                }
          
                if (YValue > 600 && FastChng == 1) {  // RH - if joystick down is held for > the fast delay time, then enter fast change mode
                  if ((millis() - SetTime) > FastDelay) {
                    FastChng = 2;
                    SetTime = millis();  // update the set time
                  }
                }
          
                if (YValue < 400 && FastChng == 2) {  // RH - we are in fast change mode, so we want to update the values quickly, proportional between
                  if ((millis() - SetTime) > (LongInt - (400 - YValue) * (LongInt - ShortInt) / 400)) {  // ShortInt and LongInt based on joystick deflection
                    TurnNr = TurnNr + 1;
                    SetTime = millis();
                    lcd.clear();
                  }
                }
          
                if (YValue > 600 && FastChng == 2) {  // RH - same for joystick down in fast change mode
                  if ((millis() - SetTime) > (LongInt - (YValue - 600) * (LongInt - ShortInt) / 400)) {
                    TurnNr = TurnNr - 1;
                    SetTime = millis();
                    lcd.clear();
                  }
                }
                
                if (TurnNr < 1){  //Min allowable amount of turns
                  TurnNr = 1;
                }
                if (TurnNr > 200){  //Max allowable amount of turns
                  TurnNr = 200;
                }
              }
          
              if (SwMenu == 3){  //Program started
                MaxSwMenu = 1;
                StepPoll = 32 * rolePerMinute;  // RH - set polling rate StepPoll = number of steps per second (roughly)
                lcd.setCursor(0, 0);
                lcd.print("Program started");
                lcd.setCursor(0, 1);
                lcd.print("Turns done: ");
                lcd.print(CurrentTurn);
          
                if (CurrentTurn < TurnNr){ 
                  myStepper.setSpeed(rolePerMinute);
                  Cntr = 0;  // RH - initialize step counter
                  while ((Cntr <= FullRev) && (CinCancelFlag == 0)) { 
                    myStepper.step(StepPoll);  // RH - breaking up plate rotation into roughly 1 sec intervals to check for cancel
                    Cntr = Cntr + StepPoll;
                    SwValue = digitalRead(SW_pin);
                    if (SwValue == 0 && BtnCancelFlag == 0) {  // RH - check if button is pressed, if so start timing for cancel
                      BtnCancelFlag = 1;
                      SetTime = millis();
                    }
                    if (SwValue == 1 && BtnCancelFlag == 1) {  // RH - if button released before BtnDelay, then reset button flag
                      BtnCancelFlag = 0;
                    }
                    if (SwValue == 0 && BtnCancelFlag == 1 && ((millis() - SetTime) > BtnDelay)) {  // RH - button has been held for > BtnDelay, so cancel operation
                      CinCancelFlag = 1;
                      CurrentTurn = TurnNr;  
                    }
                  }
                  if (CinCancelFlag == 0) {  // RH - operation not cancelled, so continue
                    myStepper.step(FullRev + StepPoll - Cntr);  // RH - need to move remainder of steps for 1 full turn due to kickout from while loop
                    CurrentTurn = CurrentTurn + 1;
                    lcd.setCursor(0, 1);
                    lcd.print("Turns done: ");
                    lcd.print(CurrentTurn);
                  }
                }
          
                if (CurrentTurn == TurnNr){  //If the current turn is equal to the amount of thurns that need to be turned -> program finished (RH - or cancelled)
                  lcd.setCursor(0, 0);
                  if (CinCancelFlag == 0) {
                    lcd.print("Program finished");
                  }
                  else {
                    lcd.clear();
                    lcd.print("Program cancel");
                  }
                  delay(3000);
                  lcd.clear();  //Reset
                  CurrentTurn = 0;
                  PhotoNr = 1;
                  rolePerMinute = 15;
                  SwMenu = 0;
                  MaxSwMenu = 0;
                  CinCancelFlag = 0;
                  Steps = 0;
                }
              }
              break;
              
    case 2: //***********************************************Menu2***********************************************//
          if (SwMenu == 0){
            lcd.setCursor(0,0);
            lcd.print("<-");
            lcd.setCursor(3, 0);
            lcd.print("Manual");
            lcd.setCursor(3,1);
            lcd.print("control");
          }
      
           if (SwMenu == 1){  //Entered menu2
            lcd.setCursor(0, 0);
            lcd.print("motor speed");
            lcd.setCursor(7, 1);
            lcd.print(rolePerMinute);
      
            if (FastChng == 0) {  // RH - if not in fast change mode, update set time
              SetTime = millis();
            }
            
            if (YValue < 400 && Flag5 == 0){ // joystick up -> Add 1 RPM
              rolePerMinute = rolePerMinute + 1;
              Flag5 = 1;
              FastChng = 1;
              lcd.clear();
            }
            if (YValue > 600 && Flag5 == 0){ // joystick down -> Subtract 1 RPM
              rolePerMinute = rolePerMinute - 1;
              Flag5 = 1;
              FastChng = 1;
              lcd.clear();
            }
            if (YValue > 399 && YValue < 599 && Flag5 == 1){  //if the Y-axis is back at it's neutral possition -> Flag3 = 0 -> Prevents constant adding or subtracting of 1
              Flag5 = 0;
              FastChng = 0;
            }
      
            if (YValue < 400 && FastChng == 1) {  // RH - if joystick up is held for > the fast delay time, then enter fast change mode
              if ((millis() - SetTime) > FastDelay) {
                FastChng = 2;
                SetTime = millis();  // update the set time
              }
            }
      
            if (YValue > 600 && FastChng == 1) {  // RH - if joystick down is held for > the fast delay time, then enter fast change mode
              if ((millis() - SetTime) > FastDelay) {
                FastChng = 2;
                SetTime = millis();  // update the set time
              }
            }
      
            if (YValue < 400 && FastChng == 2) {  // RH - we are in fast change mode, so we want to update the values at the LongInt interval
              if ((millis() - SetTime) > LongInt) {  // (using fixed LongInt since not that many values to cycle through)
                rolePerMinute = rolePerMinute + 1;
                SetTime = millis();
                lcd.clear();
              }
            }
      
            if (YValue > 600 && FastChng == 2) {  // RH - same for joystick down in fast change mode
              if ((millis() - SetTime) > LongInt) {
                rolePerMinute = rolePerMinute - 1;
                SetTime = millis();
                lcd.clear();
              }
            }
            
            if (rolePerMinute < 1){  //Min allowable RPM
              rolePerMinute = 1;
            }
            if (rolePerMinute > 17){  //Max allowable RPM
              rolePerMinute = 17;
            }
      
            if (XValue > 400 ){  //if the joystick is pushed to the right side and the neutral flag is 0 then 1 will be added to the menu number (purpose of the flag -> see comment Flag1 above)
              Steps = Steps + 1;
              myStepper.setSpeed(rolePerMinute);
              myStepper.step(Steps);
              lcd.setCursor(14, 1);
              lcd.print("<-");
              Flag6 = 1;
            }
      
            if (XValue < 600 ){  //if the joystick is pushed to the left side and the neutral flag is 0 then 1 will be subtracted from the menu number (purpose of the flag -> see comment Flag1 above)
              Steps = Steps + 1;
              myStepper.setSpeed(rolePerMinute);
              myStepper.step(-Steps);
              lcd.setCursor(0, 1);
              lcd.print("->");
              Flag6 = 1;
            }
      
            if (XValue > 399 && XValue < 599){  //if the Y-axis is back at it's neutral possition -> Flag3 = 0 -> Prevents constant adding or subtracting of 1
              Steps = 0;
              if (Flag6 == 1){
                lcd.clear();
                Flag6 = 0;  //This flag is only active during 1 program cycle to clear the lcd
              }
            }
           }
      
           if (SwMenu == 2){  //Leave menu 2 when button is pressed
             lcd.clear();
             CurrentTurn = 0;
             PhotoNr = 1;
             rolePerMinute = 15;
             SwMenu = 0;
             Steps = 0;
           }
           break;
           
    default:
           break;       
  }
  
}
    
    
