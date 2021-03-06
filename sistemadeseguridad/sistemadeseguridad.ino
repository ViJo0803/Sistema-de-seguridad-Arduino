#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <NewPing.h>
#include <NewTone.h>
#include <SoftwareSerial.h>
SoftwareSerial SIM900(10, 11); // 10 y 11

String password="2588"; //Variable to store the current password
String tempPassword=""; //Variable to store the input password
int doublecheck;
boolean armed = false;  //Variable for system state (armed:true / unarmed:false)
boolean input_pass;   //Variable for input password (correct:true / wrong:false)
boolean storedPassword = true;
boolean changedPassword = false;
boolean checkPassword = false;
int distance;
int i = 1; //variable to index an array

const byte numRows= 4; //number of rows on the keypad
const byte numCols= 3; //number of columns on the keypad
char keypressed;
//keymap defines the key pressed according to the row and columns just as appears on the keypad
char keymap[numRows][numCols]=
{
{'1', '2', '3'},
{'4', '5', '6'},
{'7', '8', '9'},
{'*', '0', '#'}
};
//Code that shows the the keypad connections to the arduino terminals
byte rowPins[numRows] = {5,4,3,2};//Rows 0 to 3
byte colPins[numCols] = {8,7,6};//Columns 0 to 2             
//initializes an instance of the Keypad class
Keypad myKeypad= Keypad(makeKeymap(keymap), rowPins, colPins, numRows, numCols);

/*--------------------------CONSTANTS-------------------------*/
const int buzzer = A1;      //Buzzer/small speaker
const int doorMagSen = 9;    //Door magnetic sensor
const int windowMagSen = 12; //Window magnetic sensors
LiquidCrystal_I2C lcd(0x27,16,2); //lcd ((RS, E, D4, D5, D6, D7)


NewPing sonar(A0,A0,9000); // Trig Echo Max distance
/*--------------------------VARIABLES------------------------*/




/**********************************************************************************/

void setup() {
   lcd.init();
   lcd.clear();
   lcd.backlight();
  pinMode(doorMagSen,INPUT_PULLUP);       //Set all magnetic sensors as input withn internal pullup resistor
  pinMode(windowMagSen,INPUT_PULLUP);
  
      SIM900.begin(19200);  //Configura velocidad del puerto serie para el SIM900
      Serial.begin(19200);  //Configura velocidad del puerto serie del Arduino
      Serial.println("OK");
      delay (1000);
}

void loop() { //Main loop
  if (armed){
    systemIsArmed();  //Run function to activate the system
  }
  else if (!armed){
    systemIsUnarmed();  //Run fuction to de activate the system
  }
}

/********************************FUNCTIONS************************************/

//While system is unarmed
void systemIsUnarmed(){
  int screenMsg=0;
  lcd.clear();                  //Clear lcd
  unsigned long previousMillis = 0;           //To make a delay by using millis() function
  const long interval = 5000;           //delay will be 5 sec. 
                          //every "page"-msg of lcd will change every 5 sec
  while(!armed){                  //While system is unarmed do...
    unsigned long currentMillis = millis();   //Store the current run-time of the system (millis function)
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        if(screenMsg==0){           //First page-message of lcd
        lcd.setCursor(0,0);
        lcd.print("Alarma Desactivada");
        lcd.setCursor(0,1);
        lcd.print("----------------");
        screenMsg=1;
        }
        else{                 //Second page-message of lcd
          lcd.setCursor(0,0);
          lcd.print("# activar alarma        ");
          lcd.setCursor(0,1);
        lcd.print("* enter");
        screenMsg=0;
        }
      }
    keypressed = myKeypad.getKey();       //Read the pressed button
    if (keypressed =='#'){            //If A is pressed, activate the system
      NewTone(buzzer,500,200);
      systemIsArmed();            //by calling the systemIsArmed function
    }
    else if (keypressed =='B'){//If B is pressed, change current password
      doublecheck=0;
      NewTone(buzzer,500,200);
      storedPassword=false;
      if(!changedPassword){         //By calling the changePassword function
        changePassword();
      }
    }
  }
}

//While system is armed
void systemIsArmed(){               
  lcd.clear();
  int count=10;               //Count 10sec before activate the system
  unsigned long previousMillis = 0;         
  const long interval = 1000; 
  while(!armed){    
    distance = sonar.ping_cm(); //Store distance from sensor only for first time
    //While system is unarmed - for 10sed do...
    lcd.setCursor(0,0);
    lcd.print(" El sistema se ");      //Print message to lcd with 10 sec timer
    lcd.setCursor(0,1);
    lcd.print("Activara en ");
    unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        //Screen counter 10sec
        if (count>1){
        count--;            //Countdown timer
        }
        else{
          armed=true;           //Activate the system!
        break;
        }
      }
    lcd.setCursor(12,1);
    lcd.print(count);           //show the timer at lcd second line 13 possition 
  }
  while (armed){                //While system is armed do...
    lcd.setCursor(0,0);
    lcd.print("Alarma activada!");
    lcd.setCursor(0,1);
    lcd.print("----------------");
    int door = digitalRead(doorMagSen);   //Read magnetic sensros and ultrasonic sensor
    int window = digitalRead(windowMagSen);
    int curr_distanse = sonar.ping_cm();

    //Check values
    if (window==HIGH){
      alarmFunction(); //Call alarm!
       
    } 
    if (door==HIGH){
      alarmFunction(); //Disarm the system with correct password
       
    }
        //Ultrasonic sensor code
    if (curr_distanse < (distance -5)){ //Check distanse (minus 5 for safety) with current distanse
      alarmFunction();
       
    }
  }
}
//Door is opend, unlcok the system!
void unlockPassword() {
  int count=21;             //20 sec for alarm!
  retry:                  //label for goto, retry in case of wrong password
    tempPassword="";            //reset temp password (typing...)
  lcd.clear();              //clear lcd
  i=6;                  //variable to put * while typing pass
  unsigned long previousMillis = 0;       
  const long interval = 1000;
  boolean buzzerState = false;      //variable to help us make  a beep NewTone
  while(!checkPassword){          //While waiting for correct password do...
    unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis; //play beep NewTone every 1 sec
        if (!buzzerState){
          NewTone(buzzer, 700);
          buzzerState=true;
        }
        else{
          noNewTone(buzzer);
          buzzerState=false;
        }
        if (count>0){           //Screen counter 20sec
        count--;
        }
        else{
          alarmFunction();      //Times is up, ALARM!
          break;
        }
      }
    keypressed = myKeypad.getKey();
    lcd.setCursor(0,0);
    lcd.print("ALARM IN: "); 
    //For screen counter - 20sec
    if (count>=10){
      lcd.setCursor(14,0);
      lcd.print(count);       //print countdown timer at lcd
    }
    else{               //catch '0'bellow 10 (eg 09)
      lcd.setCursor(14,0);
      lcd.print(" ");
      lcd.print(count);
    }
    lcd.setCursor(0,1);
    lcd.print("Contrase??a");
    if (keypressed != NO_KEY){      //Accept only numbers and * from keypad
      if (keypressed == '0' || keypressed == '1' || keypressed == '2' || keypressed == '3' ||
      keypressed == '4' || keypressed == '5' || keypressed == '6' || keypressed == '7' ||
      keypressed == '8' || keypressed == '9' ){
        tempPassword += keypressed;
        lcd.setCursor(i,1);
        lcd.print("*");       //Put * on lcd
        i++;
        NewTone(buzzer,500,200);    //Button NewTone
      }
      else if (keypressed == '1721'){  //Check for password
        if (password==tempPassword){//If it's correct unarmed the system
          armed=false;
          NewTone(buzzer,700,500);
          break;
        }
        else{           //if it's false, retry
          tempPassword="";
          NewTone(buzzer,200,200);
          delay(300);
          NewTone(buzzer,200,200);
          delay(300);
          goto retry;
        }
      }
    }
  } 
  
}

//Alarm
void alarmFunction(){
 
  retry: //label for goto
  tempPassword="";
  lcd.clear();
  i=6;
  unsigned long previousMillis = 0;       
  const long interval = 500;
  boolean buzzerState = false;
  
  SIM900.println("ATD989575890;");  //Comando AT para realizar una llamada

      SIM900.println("ATH");  // Cuelga la llamada
      
  while(!checkPassword){          //Waiting for password to deactivate the alarm...
    unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis; //Play a beep NewTone every 0.5 second
        if (!buzzerState){
          NewTone(buzzer, 700);
      
          buzzerState=true;
        }
        else{
          noNewTone(buzzer);
          buzzerState=false;
        }
      }
    keypressed = myKeypad.getKey();
    lcd.setCursor(0,0);
    lcd.print("  !!! ALARMA !!! "); 
    lcd.setCursor(0,1);
    lcd.print("PASS>");
    if (keypressed != NO_KEY){      //Accept only numbers and *
      if (keypressed == '0' || keypressed == '1' || keypressed == '2' || keypressed == '3' ||
      keypressed == '4' || keypressed == '5' || keypressed == '6' || keypressed == '7' ||
      keypressed == '8' || keypressed == '9' ){
        tempPassword += keypressed;
        lcd.setCursor(i,1);
        lcd.print("*");
        i++;
      }
      else if (keypressed == '*'){
        if (password==tempPassword){
          armed=false;
          NewTone(buzzer,700,500);
          break;
        }
        else{
          tempPassword="";
          NewTone(buzzer,200,200);
          delay(300);
          NewTone(buzzer,200,200);
          delay(300);
          goto retry;
        }
      }
    }
  } 
}
//Change current password
void changePassword(){
  retry: //label for goto
  tempPassword="";
  lcd.clear();
  i=1;
  while(!changedPassword){        //Waiting for current password
    keypressed = myKeypad.getKey();   //Read pressed keys
    lcd.setCursor(0,0);
    lcd.print("Cambiar contrase??a");
    lcd.setCursor(0,1);
    lcd.print(">");
    if (keypressed != NO_KEY){
      if (keypressed == '0' || keypressed == '1' || keypressed == '2' || keypressed == '3' ||
      keypressed == '4' || keypressed == '5' || keypressed == '6' || keypressed == '7' ||
      keypressed == '8' || keypressed == '9' ){
        tempPassword += keypressed;
        lcd.setCursor(i,1);
        lcd.print("*");
        i++;
        NewTone(buzzer,800,200);        
      }
      else if (keypressed=='#'){
        break;
      }
      else if (keypressed == '*'){
        i=1;
        if (password==tempPassword){
          storedPassword=false;
          NewTone(buzzer,500,200);
          newPassword();          //Password is corrent, so call the newPassword function
          break;
        }
        else{               //Try again
          tempPassword="";
          NewTone(buzzer,500,200);
          delay(300);
          NewTone(buzzer,500,200);
          delay(300);
          goto retry;
        }
      }
    }
  }
}
String firstpass;
//Setup new password
void newPassword(){
  tempPassword="";
  changedPassword=false;
  lcd.clear();
  i=1;
  while(!storedPassword){
    keypressed = myKeypad.getKey();   //Read pressed keys
    if (doublecheck==0){
      lcd.setCursor(0,0);
      lcd.print("SET NEW PASSWORD");
      lcd.setCursor(0,1);
      lcd.print(">");
    }
    else{
      lcd.setCursor(0,0);
      lcd.print("One more time...");
      lcd.setCursor(0,1);
      lcd.print(">");
    }
    if (keypressed != NO_KEY){
      if (keypressed == '0' || keypressed == '1' || keypressed == '2' || keypressed == '3' ||
      keypressed == '4' || keypressed == '5' || keypressed == '6' || keypressed == '7' ||
      keypressed == '8' || keypressed == '9' ){
        tempPassword += keypressed;
        lcd.setCursor(i,1);
        lcd.print("*");
        i++;
          NewTone(buzzer,800,200);
      }
      else if (keypressed=='#'){
        break;
      }
      else if (keypressed == '*'){
        if (doublecheck == 0){
          firstpass=tempPassword;
          doublecheck=1;
          newPassword();
        }
        if (doublecheck==1){
          doublecheck=0;
          if (firstpass==tempPassword){
            i=1;
            firstpass="";
            password = tempPassword; // New password saved
            tempPassword="";//erase temp password
            lcd.setCursor(0,0);
            lcd.print("PASSWORD CHANGED");
            lcd.setCursor(0,1);
            lcd.print("----------------");
              storedPassword=true;
              NewTone(buzzer,500,400);
              delay(2000);
              lcd.clear();
              break;
          }
          else{
            firstpass="";
            newPassword();
          }
        }
      } 
    }
  }
}

void llamar()
   {
      SIM900.println("ATD989575890;");  //Comando AT para realizar una llamada
      delay(300000);  // Espera 30 segundos mientras realiza la llamada
      SIM900.println("ATH");  // Cuelga la llamada
      delay(1000);
   }

