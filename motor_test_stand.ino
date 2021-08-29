
#include "HX711_2.h"

#include <stdarg.h>

#include <Servo.h>

const int esc_pin = 10;  // TODO
Servo myservo;


// HX711 circuit wiring
const int LOADCELL_1_DOUT_PIN = 8;
const int LOADCELL_1_SCK_PIN = 9;

//const int LOADCELL_2_DOUT_PIN = 4;
//const int LOADCELL_2_SCK_PIN = 6;

const int VOLTAGE_READ_PIN = A1;
const int CURRENT_READ_PIN = A0;

const int READINGS_PER_AVERAGE = 3;


const float force_calibration_factor = 1.0;


HX711 scale1;
HX711 scale2;


void setup_scale(HX711& scale, int dout_pin, int sck_pin) {
  // Initialize library with data output pin, clock input pin and gain factor.
  // Channel selection is made by passing the appropriate gain:
  // - With a gain factor of 64 or 128, channel A is selected
  // - With a gain factor of 32, channel B is selected
  // By omitting the gain factor parameter, the library
  // default "128" (Channel A) is used here.
  scale.begin(dout_pin, sck_pin);

  Serial.println("Before setting up the scale:");
  Serial.print("read: \t\t");
  Serial.println(scale.read());      // print a raw reading from the ADC

  Serial.print("read average: \t\t");
  Serial.println(scale.read_average(5));   // print the average of 20 readings from the ADC

  Serial.print("get value: \t\t");
  Serial.println(scale.get_value(5));   // print the average of 5 readings from the ADC minus the tare weight (not set yet)

  Serial.print("get units: \t\t");
  Serial.println(scale.get_units(5), 1);  // print the average of 5 readings from the ADC minus tare weight (not set) divided
  // by the SCALE parameter (not set yet)

  scale.set_scale(2280.f);                      // this value is obtained by calibrating the scale with known weights; see the README for details
  scale.tare();               // reset the scale to 0

  Serial.println("After setting up the scale:");

  Serial.print("read: \t\t");
  Serial.println(scale.read());                 // print a raw reading from the ADC

  Serial.print("read average: \t\t");
  Serial.println(scale.read_average(5));       // print the average of 20 readings from the ADC

  Serial.print("get value: \t\t");
  Serial.println(scale.get_value(5));   // print the average of 5 readings from the ADC minus the tare weight, set with tare()

  Serial.print("get units: \t\t");
  Serial.println(scale.get_units(5), 1);        // print the average of 5 readings from the ADC minus tare weight, divided
  // by the SCALE parameter set with set_scale
}



void read_and_print(HX711& scale) {
  scale.power_up();
  Serial.print("one reading:\t");
  Serial.print(scale.get_units(), 1);
  Serial.print("\t| average:\t");
  Serial.println(scale.get_units(10), 1);
  scale.power_down();              // put the ADC in sleep mode
}



float read_scale(HX711& scale) {
  return scale.get_units(1);
}



void setup() {
  // put your setup code here, to run once:

  myservo.attach(esc_pin);
  myservo.write(90);

  Serial.begin(115200);

  Serial.println("HX711 Demo");

  Serial.println("Initializing the scale");

  setup_scale(scale1, LOADCELL_1_DOUT_PIN, LOADCELL_1_SCK_PIN);
//  setup_scale(scale2, LOADCELL_2_DOUT_PIN, LOADCELL_2_SCK_PIN);

  read_scale(scale1);
//  read_scale(scale2);


  Serial.println("Readings:");
  Serial.setTimeout(25);

  pinMode(13, INPUT);
}

void read_iter() {

  scale1.power_up();

  float net_force = 0;
  float voltage = 0;
  float current = 0;

  float scale_a = 0;
  float scale_b = 0;

  for (int i = 0; i < READINGS_PER_AVERAGE; i++) {
    scale_a = read_scale(scale1);
    net_force += scale_a * force_calibration_factor;

    voltage += analogRead(VOLTAGE_READ_PIN) * (3.3 / 1024.0) * (50.0 / 12.0) * (11.5 / 7.77);
    current += analogRead(CURRENT_READ_PIN) * (3.3 / 1024.0) / 3.4 * 44.0;
//    voltage += analogRead(VOLTAGE_READ_PIN);  // TODO: change this calibration value if another voltage divider is instaled
//    current += analogRead(CURRENT_READ_PIN);
  }  

  net_force /= READINGS_PER_AVERAGE;
  voltage /= READINGS_PER_AVERAGE;
  current /= READINGS_PER_AVERAGE;
  
  Serial.print(voltage * current, 5);
  Serial.print(" ");
  Serial.print(net_force, 2);
  Serial.print(" ");
  Serial.print(voltage, 3);
  Serial.print(" ");
  Serial.print(current, 3);
  Serial.println();  
}

void do_ramp(int maxPower) {
  bool reverse = false;
  if (maxPower < 90) {
    reverse = true;
  }
  Serial.print("pwm ");
      Serial.print("power ");
      Serial.print("net_force ");
      Serial.print("voltage ");
      Serial.print("current ");
      Serial.println();
      for (int i = 45; i <= maxPower; i++) {
          if (reverse) {
            myservo.write(180 - i);
          }
          else {
            myservo.write(i);
          }
          unsigned long starttime = millis();
          while (millis() < starttime + 1000) {
            Serial.print(i);
            Serial.print(" ");
            read_iter();
            int input = Serial.parseInt();
            if (input > 0) {
              // this is a safety thing, so the ramp can be canceled any time
              return;
            }
          }
      }
    }

void loop() {

  if(Serial.available() > 0) {
    int maxPower = Serial.parseInt();
    if (maxPower > 0) {
      do_ramp(maxPower);
    }
  // turn off the motor
  myservo.write(45);
  }
}
