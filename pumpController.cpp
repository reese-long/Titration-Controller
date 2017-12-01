//RPM = 2.2135*(mL/min) - 0.0599
#include <Keypad.h>
#include <Stepper.h>
#include <LiquidCrystal_I2C.h>
#include "EEPROM.h"
#include <Wire.h>
#include <math.h>
#include "Adafruit_MCP23017.h"

const int stepsPerRevolution = 200;

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
byte rowPins[ROWS] = {2, 3, 4, 5}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {6, 7, 12, 13}; //connect to the column pinouts of the keypad

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
//CHANGE FOR THE FORK
const byte start_pause_button = 16;          //16                    //starts and pauses action of syringe pump
const byte reset_button = 14;             //14                       //reset all values to 0;
const byte set_button = 15;              //15                       //set values for each portion of titration cycle
const byte buzzer_pin = 17;

int rateArray [5] = {0, 0, 0, 0, 0};
int volumeArray [5] = {0, 0, 0, 0, 0};
int rpmArray [5] = {0, 0, 0, 0, 0};
long int runtimeMillisArray [5] = {0, 0, 0, 0, 0};

bool earlyExit = false;
bool nowRunning = false;


Stepper myStepper(stepsPerRevolution, 8, 9, 10, 11);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
Adafruit_MCP23017 mcp;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {

  Serial.begin(9600);
  Wire.begin();
  lcd.begin();
  lcd.clear();
  mcp.begin();


  const int stepsPerRevolution = 200;  // change this to fit the number of steps per revolution

  pinMode(start_pause_button, INPUT_PULLUP);
  pinMode(reset_button, INPUT_PULLUP);
  pinMode(set_button, INPUT_PULLUP);
  pinMode(buzzer_pin, OUTPUT);

  for (int i = 0; i < 10; i++)
  {
    mcp.pinMode(i, OUTPUT);          //set the bar led graph as outputs using port expander (mcp23017)
  }

  for (int i  = 0; i < 10; i++) { //turn on all 10 leds of led bar
    mcp.digitalWrite(i, HIGH);
    delay(100);
  };

  startup_sequence();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop()
{
  buttonPoll();
  // debug();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void buttonPoll()

{

  if (digitalRead(set_button) == LOW)
  {
    chirp_buzzer();
    delay(100);
    set_button_pressed();
  }


  if (digitalRead(start_pause_button) == LOW)
  {
    chirp_buzzer();
    nowRunning = true;    delay(100);
    start_titration();
  }


  if (digitalRead(reset_button) == LOW)
  {
    chirp_buzzer();
    delay(100);
    reset_sequence();
  }


}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ledBar(long int runtime, byte &ledCount, byte &multCount, long int timePerSegment)
{


  if (runtime > (timePerSegment * multCount))
  {
    mcp.digitalWrite(ledCount, LOW);
    ledCount --;
    multCount++;
  }

}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void start_titration()
{
  long int currentMillis = 0;

  for (int i = 0; i < 5 ; i++)                              //5 different sections of titration cycle
  {
    if (volumeArray[i] == 0)      //if the segment is undefined
    {
      continue;
    }
    byte ledCount = 9;
    byte multCount = 1;
    long int timePerLedSegment = round((float(runtimeMillisArray[i]) / 10));
    lcd_display(i);  //display settings for the current segment of the titration
    delay(200);
    for (int i  = 0; i < 10; i++) {                         //turn on all 10 leds of led bar
      mcp.digitalWrite(i, HIGH);
      delay(100);
    };


    // displays rate and volume to LCD
    long int howLongRunningMil = 0;
    bool cyclePaused = false;
    myStepper.setSpeed(rpmArray[i]);
    currentMillis = millis();
    while (howLongRunningMil < runtimeMillisArray[i])
    {
      if (nowRunning)
      {
        if (millis() >= currentMillis + 1000)
        {
          currentMillis = millis();
          howLongRunningMil += 1000;
          Serial.print("howlongrunMillis =  "); Serial.print(howLongRunningMil);
        }

        myStepper.step(1);                           //stepper motor takes a single step
        ledBar(howLongRunningMil, ledCount, multCount, timePerLedSegment);

        if (digitalRead(start_pause_button) == LOW)
        {
          chirp_buzzer();
          nowRunning = false;                        //allows user to pause the run
          delay (400);                                //debouce
        }
      }
      if (digitalRead(start_pause_button) == LOW)
      {
        chirp_buzzer();
        nowRunning = true;                        //allows user to resume the run
        delay(400);                //debouce
      }
      if (digitalRead(reset_button) == LOW)     //user can reset all parameters and exit run
      {

        reset_sequence();
        return;
      }
    }
    howLongRunningMil = 0;      //reset time running
    lcd.clear();
    //if (i != 4 ||) { // OR this if(volumeArray[i+1] != 0)
     for (int i  = 10; i >= 0; i--) {     //turn on all 10 leds of led bar
    mcp.digitalWrite(i, LOW);
    delay(30);
  };
      lcd.setCursor(3, 0);
      lcd.print("SEGEMENT "); lcd.print(i + 1);
      lcd.setCursor(4, 1);
      lcd.print("COMPLETE");
      for (int i = 0; i < 4; i++) {
        tone(buzzer_pin, 440);
        delay (150);
        noTone(buzzer_pin);
        delay(100);
      }
      delay(500);
    }
  }
  lcd.clear();
  for (int i  = 10; i >= 0; i--) {     //turn on all 10 leds of led bar
    mcp.digitalWrite(i, LOW);
    delay(60);
  };
  lcd.setCursor(4, 0);
  lcd.print("TITRATION");
  lcd.setCursor(5, 1);
  lcd.print("COMPLETE ");
  for (int i = 0; i < 2; i++)
  {
  tone(buzzer_pin, 261);
  delay(200);
  noTone(buzzer_pin);
  tone(buzzer_pin, 349);
  delay(200);
  noTone(buzzer_pin);
  tone(buzzer_pin, 440);
  delay(250);
  noTone(buzzer_pin);
  delay(10);
  }
   tone(buzzer_pin, 261);
  delay(250);
  noTone(buzzer_pin);
  tone(buzzer_pin, 349);
  delay(300);
  noTone(buzzer_pin);
  tone(buzzer_pin, 440);
  delay(400);
  noTone(buzzer_pin);
}




///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void set_button_pressed() //used to set parameter for up to five segements of the titration
{
  for (int i = 0 ; i < 5 ; i++)
  {
    Serial.println("IM IN THE SETBUTTON FX");

    lcd_display(i);     //display rate and volume data to LCD for the current segment
    if (digitalRead(reset_button) == LOW)     //user has option to reset all values
    {
      chirp_buzzer();
      reset_sequence();
    }
    rateArray[i] = keypad_input(0, 0, "mL/MIN-" + String(i + 1) + ": ", i, 0, rateArray[i]);      //insert into arrays values returned from
    volumeArray[i] = keypad_input(3, 1, "VOL-" + String(i + 1) + ": ", i, 1, volumeArray[i]);     //'keypad_input' function
    if (earlyExit) {      //if the user wants to define less than five segments of titration...
      delay(200);
      rpmArray[i] = round(2.2135 * float(rateArray[i]) - 0.0599);     //stepper motor rpm setting calculated based of rate
      runtimeMillisArray[i] = 60000L * (float(volumeArray[i]) / float(rateArray[i]));     //runtime calculated based on volume and rate
      earlyExit = false;      //reset boolean
      break;      //leave the for loop, less than five segments will be defined
    }
    rpmArray[i] = round(2.2135 * float(rateArray[i]) - 0.0599);     //equation derived from calibration expriments using
    runtimeMillisArray[i] = 60000L * (float(volumeArray[i]) / float(rateArray[i]));     //peristaltic pump
  }
  lcd_display(0);
  delay(200);
  return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void lcd_display(int index)     //displays the rate and volume to the 16x2 LCD display
{ //argument 'index' used to indicate which segment of titration
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("mL/MIN-" + String(index + 1) + ": ");
  lcd.setCursor(10, 0);
  lcd.print(rateArray[index]);
  lcd.setCursor(3, 1);
  lcd.print("VOL-" + String(index + 1) + ": ");
  lcd.setCursor(10, 1);
  lcd.print(volumeArray[index]);

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



int keypad_input(int xclear, int yclear, String blinkText, int currentIndex, int bothValuesSet, int formerValue)

{
  String valueEntered = "";
  String digitsString = "0123456789";     //used to check against keypad inputs - if present in digitsString
  bool nowEntering = true;                //concat to valueEntered
  bool blinkState = true;
  long int blinkMillis = millis();
  lcd_display(currentIndex);
  while (nowEntering) {       //loop through this function until a value has been entered and submitted
    Serial.println("IM IN THE KEYPAD FX");
    if (digitalRead(reset_button) == LOW)     //user has option to reset all values
    {
      chirp_buzzer();
      reset_sequence();
    }

    char buttonPressed = customKeypad.getKey();
    if (buttonPressed  == '*' && bothValuesSet)       //second argument (bothValuesSet) must be true to ensure
    { //user has defined both rate AND volume before early exit.
      earlyExit = true;

      for (int j = currentIndex + 1; j < 5; j++)      //press asterisk key (instead of pound) to early exit
      { //(exit without defining rate/volume for all five segments of titration)
        rateArray [j] = 0;                            //for loop sets array values for proceeding segments to zero
        volumeArray [j] = 0;
        rpmArray [j] = 0;
        runtimeMillisArray [j] = 0;

      }
      tone(buzzer_pin, 440);
      delay(100);
      noTone(buzzer_pin);
      delay(150);
      if (valueEntered != "") {
        return valueEntered.toInt();      //convert digit string to integer and return it
      }
      else {
        return formerValue;
      }
    }

    if (buttonPressed == '#')
    {
      tone(buzzer_pin, 440);
      delay(100);
      noTone(buzzer_pin);
      delay(150);
      lcd.setCursor(xclear, yclear);
      lcd.print(blinkText);
      if (valueEntered != "") {
        return valueEntered.toInt();      //convert digit string to integer and return it
      }
      else {
        return formerValue;
      }
    }
    if (buttonPressed && digitsString.indexOf(buttonPressed) > -1)      //when keypad button pressed is a number
    { //concatenate the value to the 'valueEntered' string

      lcd.setCursor(xclear + (blinkText.length() + 1), yclear);
      lcd.print("     ");
      valueEntered += buttonPressed;
      chirp_buzzer();
      lcd.setCursor(xclear + blinkText.length(), yclear);
      lcd.print(valueEntered);
    }




    if (millis() >= blinkMillis + 400) {
      lcd.setCursor(xclear, yclear);

      if (blinkState) {
        if (yclear == 1) {
          lcd.print("      ");      //different clear lengths required for top row(rate) and
        }                           //bottom row (volume)
        else {
          lcd.print("         ");
        }
        blinkMillis = millis();       //update timer
        blinkState = !blinkState;     //toggle this to switch between printing text/clearing
      }
      else {
        lcd.print(blinkText);
        blinkMillis = millis();     //update timer
        blinkState = !blinkState;     //toggle this to switch between printing text/clearing
      }
    }
  }


}

void chirp_buzzer()     //a short chirping noise to give feedback when bottons/keypad pressed

{
  tone(buzzer_pin, 440) ;
  delay(30);
  noTone(buzzer_pin) ;
}



void startup_sequence()

{
  lcd.setCursor(1, 0);
  lcd.print("TITRATION PUMP");
  lcd.setCursor(3, 1);
  lcd.print("CONTROLLER");
  tone(buzzer_pin, 261);
  delay(200);
  noTone(buzzer_pin);
  tone(buzzer_pin, 349);
  delay(200);
  noTone(buzzer_pin);
  tone(buzzer_pin, 440);
  delay(360);
  noTone(buzzer_pin);
  delay(2000);
  set_button_pressed();  //upon startup, input setting as if setting button were pressed
}



void reset_sequence()

{
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("RESET CYCLE");
  delay(1500);
  for (int i  = 0; i < 10; i++) {     //turn on all 10 leds of led bar
    mcp.digitalWrite(i, LOW);
  };

  for (int i = 0; i < 5; i++)     //set values in arrays to zero
  {
    rateArray [i] = 0;
    volumeArray [i] = 0;
    rpmArray [i] = 0;
    runtimeMillisArray [i] = 0;
  }
  set_button_pressed();     //after resetting values, define new values
}



void debug()

{
  Serial.print("start_pause_button  ");     //print the status of the three control buttons to serial monitor
  Serial.println(digitalRead(start_pause_button));
  Serial.print("set_button  ");
  Serial.println(digitalRead(set_button));
  Serial.print("reset_button  ");
  Serial.println(digitalRead(reset_button));
}