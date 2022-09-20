/*
  AD7124 Redoxmeter

  Prints out the voltage measured at pins AIN1(+)/AIN0(-)
  Input voltage must be in the range between 0 and 2.5V

  For more on AD7124, see
  http://www.analog.com/media/en/technical-documentation/data-sheets/AD7124-4.pdf

  The circuit:
  - AD7124 connected on the MOSI, MISO, SCK and /SS pins (pin 10)
  - LED active in low state connected to pin 9.

  The conversion times are as follows:
  ----------------------------------------
  | Mode      | Tconv (ms) | Fconv (Sps) |
  +-----------+------------+-------------+
  | LowPower  | 652        | 1.534       |
  | MidPower  | 330        | 3.030       |
  | FullPower | 90         | 11.11       |
  ----------------------------------------

  original example created 2018
  by epsilonrt https://github.com/epsilonrt
*/

#include <Arduino.h>
#include <ad7124.h>
#include "main.h"
#include "config.h"


// #define DEBUG

/* public variables ========================================================= */
Ad7124Chip adc;

unsigned long lastMeasurement = 0;
unsigned long lastRun = 0;


// -----------------------------------------------------------------------------
void setup() {

    // Initialize serial and wait for port to open:
    Serial.begin(115200);

    #ifdef DEBUG
    while (!Serial) {
        ;  // wait for serial port to connect. Needed for native USB port only
    }
    #endif

    logString("INFO", "AD7124 Redoxmeter");
    logString("INFO", "Startup...");

    logString("INFO", "Initializing AD7124...");
    setupADC();
    Serial.println("\tdone!");

    logString("INFO", "Initializing Pins...");
    setupPins();
    Serial.println("\tdone!");

    logString("INFO", "Startup complete, ready to redox!");

}

void loop(){


    if(millis() - lastMeasurement > measeurementInterval){
        lastMeasurement = millis();
        auto x = isRedoxTooLow();
        if((x && millis() - lastRun > pauseAfterMotorActivation) || (x && lastRun == 0)){
            activatePumpFor(2000);
            lastRun = millis();
        } else if (x && millis() - lastRun < pauseAfterMotorActivation){
            logString("WARN", "Redox too low, but pump already activated recently");
        } else {
            logString("INFO", "Redox ok");
        }
    }
    

    delay(100);



}

// -----------------------------------------------------------------------------

void setupPins(){
    pinMode(ledPin, OUTPUT);
    pinMode(motorPin, OUTPUT);

}

void setupADC(){
    // Initializes the AD7124 device, the pin /CS is pin 10 (/SS)
    adc.begin(ssPin);

    // Setting the configuration 0:
    // - use of the internal reference voltage 2.5V
    // - gain of 1 for a bipolar measurement +/- 2.5V
    adc.setConfig(0, Ad7124::RefInternal, Ad7124::Pga4, true);
    // Setting channel 0 using pins AIN6(+)/AIN7(-)
    adc.setChannel(0, 0, Ad7124::AIN6Input, Ad7124::AIN7Input);
    // Configuring ADC in Full Power Mode (Fastest)
    adc.setAdcControl(Ad7124::StandbyMode, Ad7124::LowPower, true);
    adc.setBiasPins(0x0080);
}

// -----------------------------------------------------------------------------

bool isRedoxTooLow() {
    double voltage;

    voltage = measureVoltage();
    
    doOnEveryMeasurement(voltage);

    if (voltage > setpoint) {
        return true;
    }
    return false;
}

double measureVoltage(){
    long value;
    double voltage;

    // Measuring Voltage on Channel 0 in Single Conversion Mode
    digitalWrite(ledPin, 0);
    value = adc.read(0);
    digitalWrite(ledPin, 1);

    // measure redox voltage
    if (value >= 0) {
        // If the measurement is successful, the value is converted into voltage
        voltage = Ad7124Chip::toVoltage(value, 4, 2.5, true);
    }
    return voltage;
}

void activatePumpFor(int ms) {
    doOnEveryMotorActivation();
    digitalWrite(ledPin, HIGH);
    digitalWrite(motorPin, HIGH);
    delay(ms);
    digitalWrite(motorPin, LOW);
    digitalWrite(ledPin, LOW);
}

void doOnEveryMeasurement(double voltage){
    // do something
    char buff[30];
    dtostrf(voltage, 5, 3, buff);
    logString("INFO", "Voltage: " + String(buff) + "V");
}

void doOnEveryMotorActivation(){
    // do something
    // Serial.println("Motor has been activated");
    logString("INFO", "Motor has been activated");
}

void logString(String severity, String message){
    Serial.print("[" + severity + "]");
    Serial.print(": ");
    Serial.println(message);
}