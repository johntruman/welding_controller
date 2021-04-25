#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <U8g2lib.h>

#define MAX_PULSE_INTERVAL      2500          //max pulse length in micros => slowest motor rotation
#define MIN_PULSE_INTERVAL      10            //min pulse length in micros => fastest motor rotation
#define MIN_SPEED MAX_PULSE_INTERVAL
#define MAX_SPEED MIN_PULSE_INTERVAL 
#define ACCEL                   100           //The amount the current speed is changed as it ramps up to the traget speed
#define CW                      1             //spin direction clockwise = 1
#define CCW                     0             //spin direction counter-clockwise = 0

//Pinouts
#define ESTOP                   2             //Emergency Stop
#define ENA_PIN                 3             //Pin connected to motor driver's ENA pin
#define DIR_PIN                 4             //Pin connected to motor driver's DIR pin
#define PUL_PIN                 LED_BUILTIN   //5 //Pin connected to motor driver's PUL pin
#define START                   6             //Start/Stop button
#define ENC_A                   7             //Rotary encoder pin A
#define ENC_B                   8             //Rotary encoder pin B
#define LIMIT_SW                9             //Limit switch
#define LCD_CS                  10            //LCD SPI Pin
#define LCD_DATA                11            //LCD SPI Pin
#define LCD_RESET               12            //LCD SPI Pin
#define LCD_CLK                 13            //LCD SPI Pin

//u82g driver constructor for ST7920 in SPI mode
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, LCD_CLK, LCD_DATA, LCD_CS, LCD_RESET);

#define ENCODER_DO_NOT_USE_INTERRUPTS
#include <Encoder.h>
Encoder Enc(ENC_A, ENC_B);
long enc_pos  = -999;

bool pulseState = LOW;
bool spinDir = CW;
unsigned long currentSpeed = MIN_SPEED;           //the current speed
unsigned long targetSpeed = MIN_SPEED;            //Target speed that's been set via rotary encoder
bool estopped = false;
bool isActive = false;
unsigned long prevMicros = 0;
unsigned long currentMicros = micros();
unsigned long prevMillis = 0;
unsigned long currentMillis = 0;

void moveMotor(unsigned long current, unsigned long target, bool dir) { 

  digitalWrite(DIR_PIN,dir);

  //Check the current speed, and accelerate or deccelerate to target speed
  if (currentMillis - prevMillis >= 10 && current != target){
    if (current < target){
      current =+ ACCEL;
      if (current > target){
        current = target;
      }
    }
    if (current > target){
      current =- ACCEL;
      if (current < target){
        current = target;
      }
    }
  }

  if (isActive) {
    if (currentMicros - prevMicros >= current) {
      prevMicros = currentMicros;
      digitalWrite(PUL_PIN, HIGH);
      delayMicroseconds(20);      
      digitalWrite(PUL_PIN, LOW);
    }
  } else {    
    digitalWrite(PUL_PIN, LOW);
    return;
  }
}

void softReset(){
  //Stuff to reset
  currentSpeed = MIN_SPEED;
  targetSpeed = MIN_SPEED;
  digitalWrite (PUL_PIN,LOW);  
  digitalWrite (ENA_PIN,LOW);
  estopped = false;
}

void eStop(){  
  estopped = true;
}

void checkControls() {
  //Check if START button was pressed
  if (digitalRead(START)==LOW){
    if (!isActive){   
      isActive = true;     
    } else {
      isActive = false;
    }
    currentSpeed = MIN_SPEED;
    delay(300);
  }

  //Check if ESTOP was pressed
  if (estopped) {
    softReset();
  }
  
  //Read Encoder
  long newPos = Enc.read();
  if (newPos != enc_pos) {
    enc_pos = newPos;

    //Set target speed
    targetSpeed = abs(enc_pos);
    if (targetSpeed < MAX_SPEED){
      targetSpeed = MAX_SPEED;
    }
    if (targetSpeed > MIN_SPEED){
      targetSpeed = MIN_SPEED;
    }

    //Set direction, CW or CCW
    if (enc_pos > 0){
      Serial.print(">>");
      spinDir = CW;
    }
    if (enc_pos < 0){
      Serial.print("<<");      
      spinDir = CCW;
    }
    Serial.println(targetSpeed);    
  }
}

void setup() {
  Serial.begin(115200);
  u8g2.begin();
  pinMode(ESTOP, INPUT_PULLUP);
  pinMode(PUL_PIN,OUTPUT);
  pinMode(DIR_PIN,OUTPUT);
  pinMode(ENA_PIN,OUTPUT);
  pinMode(LIMIT_SW,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ESTOP), eStop, FALLING);  
}

void loop() {  
  currentMicros = micros();
  currentMillis = millis();
  checkControls();
  moveMotor(currentSpeed, targetSpeed, spinDir);
}
