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

//Display Layout
#define SX 73
#define SY 40


//u82g driver constructor for ST7920 in SPI mode
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, LCD_CLK, LCD_DATA, LCD_CS, LCD_RESET);

#define ENCODER_DO_NOT_USE_INTERRUPTS
#include <Encoder.h>
Encoder Enc(ENC_A, ENC_B);
long enc_pos  = -999;

bool started = 0;
bool pulseState = LOW;
bool spinDir = CW;
bool curDir = CW;
bool endA_triggered = false;
bool endB_triggered = false;
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
bool refreshDue = false;


void(* softReset) (void) = 0; 

void readout() {
  int tx;
  int t = map(targetSpeed,MIN_STEP_RATE,MAX_STEP_RATE,1,100);
  if (t < 10) tx = 2;
  if (t >= 10 && t<100) tx = 5;
  if (t >= 100)  tx = 8;    
  u8g2.setCursor(SX-tx-9,SY+12);
  
  u8g2.setDrawColor(1);   
  u8g2.drawFrame(SX-20,SY+1,22,15); 
  if(isActive){      
    u8g2.setDrawColor(1);     
    u8g2.drawBox(SX-19,SY+2,20,13); 
    u8g2.setDrawColor(0);        
    u8g2.print(t);      
  } else {
    u8g2.setDrawColor(0);   
    u8g2.drawBox(SX-19,SY+2,20,13); 
    u8g2.setDrawColor(1);       
    u8g2.print(t);    
  }     
  //u8g2.sendBuffer();
}

void refreshLCD() {
  speed = map(targetSpeed,min_speed,max_speed,0,45);
  if(currentMillis-prevLcdMillis > 200){    
    u8g2.setDrawColor(0);   
    u8g2.drawBox(1,SY+1,SX-21,15);       
    u8g2.drawBox(SX+2,SY+1,128,15);   
    u8g2.setDrawColor(1);    
    //u8g2.clearBuffer();
    if(targetSpeed!=min_speed+max_speed){
      if(spinDir){                    
        u8g2.drawTriangle(SX+speed+2,SY,SX+speed+2,SY+16,SX+speed+10,SY+8);
      } else {
        u8g2.drawTriangle(SX-speed-20,SY,SX-speed-20,SY+16,SX-speed-27,SY+8);
      }
    }
    readout();    
    u8g2.sendBuffer();	
    prevLcdMillis = millis();
  }
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
  //***********ENDSTOP*************
  if(digitalRead(LIMIT_SW) == 0 && isActive){
    if (spinDir == CW){
      endA_triggered = true;      
      endB_triggered = false;
    } else {
      endA_triggered = false; 
      endB_triggered = true;
    }    
    isActive = false;
  } else {    
    endA_triggered = false; 
    endB_triggered = false;
  }

  //***********START***************
  if (digitalRead(START)==LOW && currentMillis - debounceMillis > 600){
    #ifdef DEBUG
    Serial.println("Start button pressed");
    #endif           
    debounceMillis = currentMillis;
    if (!isActive){   
      if(!endA_triggered && !endB_triggered){   
        isActive = true;          
        return;
      } else {
        if(endA_triggered && CCW){
          isActive = true;
          endA_triggered = false;
          return;
        }
        if(endB_triggered && CW){
          isActive = true;
          endB_triggered = false;
        }
      }      
    } else {      
      isActive = false;
      //softReset();
    }
    refreshDue = true;
    return;
  }

  //***********ESTOP***************
  if (estopped) {
    softReset();
  }
  
  //***********ENCODER***************
  long newPos = Enc.read();
  if (newPos != enc_pos) {         
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
    refreshDue = true; 

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
  u8g2.setFont(u8g2_font_profont11_mr);	

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
  if(refreshDue){
    refreshLCD();
    refreshDue = false;
  }
}