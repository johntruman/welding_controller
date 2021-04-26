#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <U8g2lib.h>

//#define DEBUG

#define STEPS_PER_REV           1600          //Steps per revolution as set in stepper driver
#define PITCH                   10            //Unit : MM     
#define MAX_SPEED               240           //in mm per second
#define MIN_SPEED               10            //in mm per second

#define MAX_STEP_RATE           5            //(1000000 / ((MAX_SPEED / PITCH) * STEPS_PER_REV))   
#define MIN_STEP_RATE           500           //(1000000 / ((MIN_SPEED / PITCH) * STEPS_PER_REV))   
#define MAX_RPM                 ((1000000 / MIN_STEP_INTERVAL) / STEPS_PER_REV)
#define MIN_RPM                 ((1000000 / MAX_STEP_INTERVAL) / STEPS_PER_REV)
 
#define ACCEL                   20            //The amount the current speed is changed as it ramps up to the traget speed
#define CW                      1             //spin direction clockwise = 1
#define CCW                     0             //spin direction counter-clockwise = 0

//Pinouts
#define ESTOP                   2             //Emergency Stop
#define ENA_PIN                 3             //Pin connected to motor driver's ENA pin
#define DIR_PIN                 4             //Pin connected to motor driver's DIR pin
#define PUL_PIN                 5             //Pin connected to motor driver's PUL pin
#define START                   6             //Start/Stop button
#define ENC_B                   7             //Rotary encoder pin B
#define ENC_A                   8             //Rotary encoder pin A
#define LIMIT_SW                9             //Limit switch
#define LCD_CS                  10            //LCD RS
#define LCD_DATA                11            //LCD RW
#define LCD_RESET               12            //LCD Reset
#define LCD_CLK                 13            //LCD E


//u82g driver constructor for ST7920 in SPI mode
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, LCD_CLK, LCD_DATA, LCD_CS, LCD_RESET);

#define ENCODER_DO_NOT_USE_INTERRUPTS
#include <Encoder.h>
Encoder Enc(ENC_A, ENC_B);
long enc_pos  = -999;

bool pulseState = LOW;
bool spinDir = CW;
bool curDir = CW;
const int min_speed = MIN_STEP_RATE;
const int max_speed = MAX_STEP_RATE;
unsigned long currentSpeed = min_speed;           //the current speed
unsigned long targetSpeed = min_speed;            //Target speed that's been set via rotary encoder
bool estopped = false;
bool isActive = false;
unsigned long prevMicros = 0;
unsigned long currentMicros = micros();
unsigned long prevMillis = 0;
unsigned long prevLcdMillis = 0;
unsigned long currentMillis = 0;
unsigned long debounceMillis = 600;
long speed = 0;

void(* softReset) (void) = 0; 

void refreshLCD() {
  int sx = 73;
  int sy = 40;
  speed = map(targetSpeed,min_speed,max_speed,0,47);
  if(currentMillis-prevLcdMillis > 200){
    u8g2.clearBuffer();	
    u8g2.drawBox(56,sy+1,16,15);
    if(targetSpeed!=min_speed+max_speed){
      if(spinDir){                    
        u8g2.drawTriangle(sx+speed,sy,sx+speed,sy+16,sx+8+speed,sy+8);
      } else {
        u8g2.drawTriangle(sx-18-speed,sy,sx-18-speed,sy+16,sx-26-speed,sy+8);
      }
    }
    u8g2.sendBuffer();	    
    prevLcdMillis = millis();
  }
  	
  //u8g2.setFont(u8g2_font_logisoso24_tf);	
  //u8g2.drawCircle(64, 32, 14, U8G2_DRAW_ALL);  
  
  
  //u8g2.drawStr(10, 50, "TEST");
  	
}

void moveMotor(unsigned long current, unsigned long target, bool dir) { 

  digitalWrite(DIR_PIN,dir);
  long delta = target - current;
  if (isActive) {
    //Check the current speed, and accelerate or deccelerate to target speed
    if (currentMillis - prevMillis >= ACCEL && delta != 0){
      if (delta > 0){
        current++;            
        if (current > target){
          current = target;
        }
      }
      if (delta < 0){
        current--;          
        if (current < target){
          current = target;
        }
      }
    }  
    currentSpeed = current;
    #ifdef DEBUG
    Serial.print ("T:");Serial.print(target);    
    Serial.print ("  C:");Serial.print(currentSpeed);
    Serial.print ("  DIR:");if(spinDir){Serial.println("CW");}else{Serial.println("CCW");}
    #endif

    if (currentMicros - prevMicros >= currentSpeed) {
      prevMicros = currentMicros;
      #ifdef DEBUG
      Serial.print("+");
      delay(100);      
      Serial.print("-");
      delay(50); 
      #else
      if(!pulseState){
        pulseState = HIGH;
      } else {
        pulseState = LOW;
      }
      #endif
    }
    digitalWrite(PUL_PIN,pulseState);
  } else {    
    digitalWrite(PUL_PIN, LOW);
    digitalWrite(ENA_PIN, LOW);
    return;
  }
}

void eStop(){  
  estopped = true;
}

void checkControls() {
  //***********START***************
  if (digitalRead(START)==LOW && currentMillis - debounceMillis > 600){
    debounceMillis = currentMillis;
    if (!isActive){   
      isActive = true;     
    } else {      
      isActive = false;
      //softReset();
    }
  }

  //***********ESTOP***************
  if (estopped) {
    softReset();
  }
  
  //***********ENCODER***************
    long newPos = Enc.read();
    if (newPos != enc_pos) {      
      refreshLCD();       
      enc_pos = newPos;
      if (enc_pos > 0){
        #ifdef DEBUG
        Serial.print("-->>");  
        #endif
        spinDir = CW;
        if (enc_pos>100) enc_pos = 100;
      }
      if (enc_pos < 0){
        #ifdef DEBUG
        Serial.print("<<--");  
        #endif
        spinDir = CCW;
        if (enc_pos< -100) enc_pos = -100;
      }

      long ts = map(enc_pos,-100,100,min_speed*-1,min_speed);
      //Set target speed
      targetSpeed = min_speed-abs(ts)+max_speed;
      #ifdef DEBUG
      Serial.println(targetSpeed);
      #endif
  } 

}

void setup() {
  //Initialize Variables
  currentSpeed = min_speed;
  targetSpeed = min_speed;
  digitalWrite (PUL_PIN,LOW);  
  digitalWrite (ENA_PIN,LOW);
  estopped = false;  
  isActive = false;

  Serial.begin(115200);
  u8g2.begin();

  //Initailize Pins
  pinMode(ESTOP, INPUT_PULLUP);
  pinMode(ENA_PIN,OUTPUT);
  pinMode(DIR_PIN,OUTPUT);
  pinMode(PUL_PIN,OUTPUT);
  pinMode(START, INPUT_PULLUP);
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(LIMIT_SW,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ESTOP), eStop, FALLING);  

  #ifdef DEBUG
  Serial.print ("T:");Serial.println(targetSpeed);    
  Serial.print ("C:");Serial.println(currentSpeed);
  #endif
}

void loop() {
  curDir = spinDir;
  currentMicros = micros();
  currentMillis = millis();
  checkControls();
  moveMotor(currentSpeed, targetSpeed, spinDir);
  if(!isActive)refreshLCD();
}
