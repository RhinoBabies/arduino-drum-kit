/******************************************************************************
Developed from:
Piezo_Vibration_Sensor.ino
Example sketch for SparkFun's Piezo Vibration Sensor
  (https://www.sparkfun.com/products/9197)
Jim Lindblom @ SparkFun Electronics
April 29, 2016

- Connect a 1Mohm resistor across the Piezo sensor's pins.
- Connect one leg of the Piezo to GND
- Connect the other leg of the piezo to A0

Vibrations on the Piezo sensor create voltage, which are sensed by the Arduino's
A0 pin. Check the serial monitor to view the voltage generated.

Development environment specifics:
Arduino 1.6.7
******************************************************************************/
//------------------------
// CONSTANTS AND DEFINES
//------------------------
#define DEBUG 0

#define BLINK_WAIT_TIME       1000  // blink the onboard LED every 1 second for pulse
#define DEFAULT_PAD_DEBOUNCE  50    // sets default for pads that haven't been configured yet
#define DEFAULT_BTN_DEBOUNCE  250
#define HIT_REG_MIN_THRESHOLD 200   // 0 - 1023, thresholds should be drum specific
#define NUM_PADS_USED         3

// Port assignments for feedback LEDs
#define BLUE_LED    5
#define GREEN_LED   6
#define RED_LED     7

// 0 1 2 3 4 5 6 (7) // Mapped output from 74HC595 for segment of LED
// A B C D E F G (.) // decimal point, bit 7 is not physically connected
// Reverse 7-seg positions for easier byte handling
// (7) 6 5 4 3 2 1 0
// (.) G F E D C B A

// Alphanumeric pin-out definitions for display on the 7-seg
#define DISP_0      0b00111111
#define DISP_1      0b00000110
#define DISP_2      0b01011011
#define DISP_3      0b01001111
#define DISP_4      0b01100110
#define DISP_5      0b01101101
#define DISP_6      0b01111101
#define DISP_7      0b00000111 // mapping is actually the same value in binary
#define DISP_8      0b01111111
#define DISP_9      0b01011111
#define DISP_A      0b01111011
#define DISP_B      DISP_8
#define DISP_C      0b00111001
#define DISP_COUNT  DISP_0, DISP_1, DISP_2

#define NUM_LEDS_USED     NUM_PADS_USED // should be same as the number of pads

//-------------------
// FUNCTION PROTOTYPES
//-------------------
int incrementDisplayNumber(void);
void readDrums(int *dbAdj);

//-------------------
// GLOBAL VARIABLES
//-------------------
const int PUSH_BUTTON = 2;
const int COLOR_LEDs[NUM_LEDS_USED] = {BLUE_LED, GREEN_LED, RED_LED};

// 74HC595 Shift Register Pin definitions
const int STORAGE_REG_LATCH_PIN = 8; // ST_CP
const int SERIAL_DATA_PIN = 11; // DS
const int SHIFT_REG_CLOCK_PIN = 12; // SH_CP

const int LED_PIN = 13;
const int PIEZO_PIN[NUM_PADS_USED] = {A0, A1, A2}; // Piezo output
const int DEBOUNCE_POT = A5;

//-------------------
// THE GOOD STUFF
//-------------------
void setup() 
{
  // Enable LEDs for hit notification
  for(int i = 0; i < NUM_LEDS_USED; i++)
    pinMode(COLOR_LEDs[i], OUTPUT);
    
  pinMode(LED_PIN, OUTPUT); // Arduino on-board LED
  
  // 74HC595 pins
  pinMode(STORAGE_REG_LATCH_PIN, OUTPUT);
  pinMode(SERIAL_DATA_PIN, OUTPUT);
  pinMode(SHIFT_REG_CLOCK_PIN, OUTPUT);
  
  pinMode(PUSH_BUTTON, INPUT_PULLUP); // Push-button to cycle digit displayed on 7-seg
  Serial.begin(57600); // baudrate for Ardrumo app
}

void loop() 
{
  int debounceAdjust[NUM_PADS_USED] = {DEFAULT_PAD_DEBOUNCE}; // Can this be stored in some type of NV memory???
  byte currNumDisplayed = 0;
  unsigned long lastPushTime = 0;
  unsigned long lastBlink = 0;
  bool LED_state = true; // true and HIGH have the same value of 0x1, false and LOW are both 0x0. Using for state inversion.
  digitalWrite(LED_PIN, LED_state);

  // How much time is saved by initializing variables globally versus putting them in their proper functions?
  // Efficiency should be optimized for speed/performance.
  
  while(1)
  {
    // Pulse the onboard LED every 'n' milliseconds
    if (millis() >= lastBlink + BLINK_WAIT_TIME)
    {
      LED_state = !LED_state;
      digitalWrite(LED_PIN, LED_state);
      lastBlink = millis();
#if DEBUG
      Serial.print(millis()/1000);
      Serial.print(" second(s)");
      Serial.println();
#endif
    }
    
    // With each push, the LED will reflect which pad's debounce is being set
    if (digitalRead(PUSH_BUTTON) == LOW && millis() >= lastPushTime + DEFAULT_BTN_DEBOUNCE)
    {
      currNumDisplayed = incrementDisplayNumber();
      lastPushTime = millis();
    }
    debounceAdjust[currNumDisplayed] = analogRead(DEBOUNCE_POT);
    
#if DEBUG
    Serial.print("db");
    Serial.print(currNumDisplayed);
    Serial.print(debounceAdjust[currNumDisplayed]);
    Serial.println();
#endif

    readDrums(debounceAdjust);
  }
}

// Cycle through the numbers that are displayed on the 7-segment LED
int incrementDisplayNumber(void)
{
  static byte i = 0xFF;
  byte alphaBits[NUM_PADS_USED] = {DISP_COUNT};

  if (++i >= NUM_PADS_USED)
    i = 0;
    
  // https://www.arduino.cc/en/Tutorial/ShftOut11
  // take the latchPin low so 
  // the LEDs don't change while you're sending in bits:
  digitalWrite(STORAGE_REG_LATCH_PIN, LOW);
  // shift out the bits:
  shiftOut(SERIAL_DATA_PIN, SHIFT_REG_CLOCK_PIN, MSBFIRST, ~alphaBits[i]); // have to invert bits because the 7-seg is common anode
  //take the latch pin high so the LEDs will light up:
  digitalWrite(STORAGE_REG_LATCH_PIN, HIGH);

  return i;
}

// Detect drum hits and display info
void readDrums(int *dbAdj)
{
  static unsigned long lastHitTime[NUM_PADS_USED] = {0};
  
  for(int i = 0; i < NUM_PADS_USED; i++)
  {
    // Read Piezo ADC value in, and convert it to a voltage
    int piezoADC = analogRead(PIEZO_PIN[i]); // takes 100 microseconds to read an analog input, so max read rate is 10k samples per second
    //piezoV = (piezoADC[i] / 1023.0) * 5.0; // range of analogRead is 0 - 1023, so multiplying by 5.0V will give the voltage
  
    // Only print/play if the hit is hard enough
    if ((piezoADC >= HIT_REG_MIN_THRESHOLD) && (millis() > (lastHitTime[i] + dbAdj[i])))
    {
      digitalWrite(COLOR_LEDs[i], HIGH);
      lastHitTime[i] = millis(); // record last time pad was hit for debouncing
      //Serial.println(piezoV); // Print the voltage.
      Serial.print(i);
      Serial.print(",");
      Serial.print(piezoADC);
      Serial.println();
      digitalWrite(COLOR_LEDs[i], LOW);
    }
  }
}
