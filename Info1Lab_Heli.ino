/*
   Author: Matthias Wolsfeld
   Date: 15.10.2020
   Last updated: 28.11:2020

   Description:
   This Sketch was written for the Informatik 1 Labor of the University of applied sciences in Karlsruhe.
   It is used to control the Main-Rotor-Model by Motoo Kondo via various input- and output-devices.
   I use an ESP32 microcontroller and A4988 stepper-driver modules
   Input-devices are: - 10k linear potentiometer for rotation speed
                      - 10k linear potentiometer as the collective
                      - joystick as the cyclic

   Output-devices are: - 2x Nema17 stepper-motors for the rotor and collective
                       - 2x Micro Servo 9g for the cyclic
                       - 0.96" OLED I2C Display


*/



// -------------------- Librarys --------------------

#include <Servo.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


// -------------------- PINs --------------------

#define poti_rotor_PIN 34  // Signal-pin of the Rotor-Potentiometer
#define poti_height_PIN 32 // Signal-pin of the Collective-Potentiometer

#define en_rotor 5         // Enable-pin A4988-Rotor
#define step_rotor 18      // Step-pin A4988-Rotor
#define dir_rotor 19       // Direction-pin A4988-Rotor

#define en_height 15       // Enable-pin A4988-Collective
#define step_height 2      // Step-pin A4988-Collective
#define dir_height 4       // Direction-pin A4988-Collective

#define endstop_height 14  // Endstop-Switch-pin Collevtive

#define joy_x 13           // Signal-pin of the Joystick-X-Axis
#define joy_y 25           // Signal-pin of the Joystick-Y-Axis

#define x_axis_PIN 26      // Signal-pin of the X-Axis Servo
#define y_axis_PIN 27      // Signal-pin of the Y-Axis Servo


// -------------------- Display --------------------

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// -------------------- Servos --------------------

Servo x_axis;
Servo y_axis;


// -------------------- Variables --------------------

volatile unsigned int poti_rotor_value;  // Input-signal of the Rotor-Potentiometer
volatile unsigned int poti_height_value; // Input-signal of the Collective-Potentiometer

volatile int height;                     // mapped value of the Collective-signal
volatile int rotorSpeed;                 // mapped value of the Rotor-signal

volatile unsigned int x_raw;             // Input-signal of the Joystick-X-Axis
volatile unsigned int y_raw;             // Input-signal of the Joystick-Y-Axis
volatile int x_value;                    // mapped value of the Joystick-X-Axis
volatile int y_value;                    // mapped value of the Joystick-Y-Axis

volatile int currentHeight;              // saves the current position of the collective

unsigned long currentMicros;
unsigned long currentMillis;
unsigned long startMicros_Rotor;
unsigned long startMicros_Servos;
unsigned long startMicros_Height;
unsigned long startMillis_Display;

unsigned int startProcedure = 1;         // Marks the first run of the programm after booting


//--------------------Setup--------------------

void setup() {


  // -------------------- Display --------------------

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Starts the communication with the OLED-Display

    for (;;);

  }


  // -------------------- pinMode --------------------

  pinMode(en_rotor, OUTPUT);
  pinMode(step_rotor, OUTPUT);
  pinMode(dir_rotor, OUTPUT);
  pinMode(en_height, OUTPUT);
  pinMode(step_height, OUTPUT);
  pinMode(dir_height, OUTPUT);
  pinMode(endstop_height, INPUT);
  pinMode(poti_rotor_PIN, INPUT);
  pinMode(poti_height_PIN, INPUT);


  // -------------------- Startconditions for Steppers --------------------

  digitalWrite(en_rotor, LOW);
  digitalWrite(dir_rotor, LOW);  //counterclockwise
  digitalWrite(en_height, LOW);
  digitalWrite(dir_height, LOW); //counterclockwise


  // -------------------- Servos --------------------

  x_axis.attach(x_axis_PIN);
  y_axis.attach(y_axis_PIN);


  // -------------------- StartScreen --------------------

  display.clearDisplay();       // Sets up and shows the Startscreen
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 15);
  display.println("Info 1 Labor");
  display.display();


  // -------------------- Timer --------------------

  startMicros_Rotor = micros();
  startMillis_Display = millis();
  startMicros_Servos = micros();

}


// -------------------- showDisplay() --------------------

void showDisplay() {                                 // Updates the OLED-Display

  if (currentMillis - startMillis_Display >= 1000) { // Checks whether the time has passed since the last function call

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);

    display.setCursor(0, 1);
    display.print("RotorSpeed: ");
    display.print(rotorSpeed);
    display.println(" %");

    display.setCursor(0, 16);
    display.print("Schub: ");
    display.print(currentHeight / 2);
    display.println(" %");

    display.setCursor(0, 31);
    display.print("X-Wert: ");
    display.println(((-20 * x_value) / 9) + 400);   // Conversion form the value in degrees to % of movement

    display.setCursor(0, 46);
    display.print("Y-Wert: ");
    display.println(((-20 * y_value) / 9) + 400);   // Conversion form the value in degrees to % of movement

    display.display();

    startMillis_Display = currentMillis;            // Resets the timer

  }
}


// -------------------- turnRotor() --------------------

void turnRotor() {                                                 // Reads the Rotor-Poti-value and maps it to a value between 0 and 100 and turns the motor in the corresponding speed

  poti_rotor_value = analogRead(poti_rotor_PIN);
  rotorSpeed = map(poti_rotor_value, 0 , 4095, 0, 100);

  if (rotorSpeed > 0) {                                            // Checks whether the value of rotorSpeed is 0 in Order to not divide by zero

    digitalWrite(en_rotor, LOW);                                   // Enables the motor

    if (currentMicros - startMicros_Rotor >= 50000 / rotorSpeed) {

      digitalWrite(step_rotor, !digitalRead(step_rotor));

      startMicros_Rotor = currentMicros;                           // Resets the timer

    }

  } else {

    digitalWrite(en_rotor, HIGH);                                  // Disables the motor when it's not in use

  }
}


// -------------------- move_X_Y() --------------------

void move_X_Y() {

  if (currentMicros - startMicros_Servos >= 50) {

    x_raw = analogRead(joy_x);
    y_raw = analogRead(joy_y);
    x_value = map(x_raw, 0, 4095, 180, 135);
    y_value = map(y_raw, 0, 4095, 180, 135);

    x_axis.write(x_value);
    y_axis.write(y_value);

    startMicros_Servos = currentMicros;

  }
}


// -------------------- move_height() --------------------

void move_height() {                                                    // Reads the Collective-Poti-value and maps it to a value between 0 and 100 and turns the motor to the corresponding position

  if (currentMicros - startMicros_Height >= 8000 ) {

    poti_height_value = analogRead(poti_height_PIN);
    height = map(poti_height_value, 0, 4095, 0, 200);

    if ((currentHeight + 1) < height && (currentHeight - 1 < height)) { // Filters parts of the analog noise out

      digitalWrite(en_height, LOW);
      digitalWrite(dir_height, LOW);                                    // Changes the direction to counterclockwise
      digitalWrite(step_height, !digitalRead(step_height));

      currentHeight++;

    } else {

      digitalWrite(en_height, HIGH);

    }

    if ((currentHeight + 2) > height && (currentHeight - 2 > height)) { // Filters parts of the analog noise out

      digitalWrite(en_height, LOW);
      digitalWrite(dir_height, HIGH);                                   // Changes the direction to clockwise
      digitalWrite(step_height, !digitalRead(step_height));

      currentHeight--;

    } else {

      digitalWrite(en_height, HIGH);

    }

    startMicros_Height = currentMicros;

  }
}


// -------------------- loop() --------------------

void loop() {

  while (startProcedure == 1) {                             // Turns the Collective-Stepper until the groundplate touches the endstop and saves it as height = 0 one time after booting

    while (digitalRead(endstop_height) == LOW) {

      digitalWrite(step_height, !digitalRead(step_height));
      delayMicroseconds(5000);

    }

    currentHeight = 0;
    startProcedure++;

  }

  // -------------------- Calls the functions --------------------

  currentMicros = micros();
  currentMillis = millis();
  turnRotor();
  move_X_Y();
  move_height();
  showDisplay();

}
