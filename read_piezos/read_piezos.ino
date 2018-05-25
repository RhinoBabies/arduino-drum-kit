/******************************************************************************
Piezo_Vibration_Sensor.ino
Example sketch for SparkFun's Piezo Vibration Sensor
  (https://www.sparkfun.com/products/9197)
Jim Lindblom @ SparkFun Electronics
April 29, 2016

- Connect a 1Mohm resistor across the Piezo sensor's pins.
- Connect one leg of the Piezo to GND
- Connect the other leg of the piezo to A0

Vibrations on the Piezo sensor create voltags, which are sensed by the Arduino's
A0 pin. Check the serial monitor to view the voltage generated.

Development environment specifics:
Arduino 1.6.7
******************************************************************************/
#define HIT_REG_MIN_THRESHOLD 200   // 0 - 1023
#define DEBOUNCE_TIME         50    // in ms
#define NUM_PADS_USED         6

const int LED_PIN = 13;
const int PIEZO_PIN[NUM_PADS_USED] = {A0, A1, A2, A3, A4, A5}; // Piezo output
int lastHitTime[NUM_PADS_USED] = {0}; //lastHit[NUM_PADS_USED] when there are more than 1 input
int currentTime = 0;

void setup() 
{
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(57600); // baudrate for Ardrumo app
}

void loop() 
{
  int piezoADC[NUM_PADS_USED] = {0};
  
  while(1)
  {
    currentTime = millis();
    
    for(int i = 0; i < NUM_PADS_USED; i++)
    {
      // Read Piezo ADC value in, and convert it to a voltage
      piezoADC[i] = analogRead(PIEZO_PIN[i]); // takes 100 microseconds to read an analog input, so max read rate is 10k samples per second
      //piezoV = piezoADC[i] / 1023.0 * 5.0; // range of analogRead is 0 - 1023, so multiplying by 5.0V will give the voltage
    
      // Only print/play if the hit is hard enough
      if ((piezoADC[i] >= HIT_REG_MIN_THRESHOLD) && (currentTime > (lastHitTime[i] + DEBOUNCE_TIME)))
      {
        digitalWrite(LED_PIN, LOW);
        lastHitTime[i] = millis();
        //Serial.println(piezoV); // Print the voltage.
        Serial.print(i);
        Serial.print(",");
        Serial.print(piezoADC[i]);
        Serial.println();
        digitalWrite(LED_PIN, LOW);
      }
    }
  }
}

