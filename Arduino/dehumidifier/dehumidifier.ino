

#define SDA A4
#define SCL A5

//#include "Wire.h"
#include <WSWire.h>

#include "SSD1306Ascii.h"
//#include "SSD1306AsciiWire.h"
//#include "SSD1306AsciiAvrI2c.h"
#include "SSD1306AsciiMasterI2C.h"

//#include <avr/wdt.h>b


#include "HIH61XX.h"



/////// GLOBALS //////

SSD1306AsciiWire oled;
//SSD1306AsciiAvrI2c oled;
#define I2C_ADDRESS 0x3C

HIH61XX hih(0x27, 255);

float hum = 50;
float temp = 5;



int oledCount = 0;

int x = 0;

#define mStandby 0
#define mCoolDown 1
#define mLow 2
#define mMed 3
#define mHigh 4
#define mTooHot 5
#define mTooCold 6
#define mSleeping 7
#define mSampling 8
#define mFanLow 9
#define mFanMed 10
#define mFanHigh 11
#define mFixedLow 12
#define mFixedMed 13
#define mFixedHigh 14

int mode = 0;
int lastMode = 0;
int previousMode = 0;

#define button1 2
#define button2 3
#define button3 16

#define water 4

#define fanL 5
#define fanM 6
#define fanH 7

#define wheel 8
#define heatFan 9

#define heater1 10
#define heater2 12

#define LED 13

#define buzzer 11

#define therm1 A0
#define therm2 A1


#define address 0x27

#define OFF HIGH
#define ON LOW

int i;

int errorHot=0;
int errorCold=0;
int errorSensor=0;

int incomingByte = 0;   // for incoming serial data

int newCommand=0;
int command;

unsigned long lastISR = 0;

unsigned long sleepTimer= 1;

bool oledOn=true;
unsigned long oledTimer= 0;

unsigned long oledTime=30000;


unsigned long sampleStart= 0;
unsigned long sinceOff= 0;

unsigned long buttonPressTime1=0;
unsigned long buttonPressTime2=0;
unsigned long buttonPressTime3=0;

bool buttonFlag1=0;
bool buttonFlag2=0;
bool buttonFlag3=0;

int offSource = 0;

unsigned long lastMillis = 0;

unsigned long startMillis=0;

unsigned long oledMillis=0;

unsigned long coolTime=0;

unsigned long offTime=1800000;

unsigned long sinceFan=0;

bool updateOLED=true;

float T1=20, T2=20;
int adc;

float toTemp(int val){

  float  V = val * (5 / 10230.0);

 // float R =10000*(1/((5/V)-1));

  //float z = log(R/100000)*2.525252525;

 // float T =10000/(25-273.15)+z;

 // float T1 = 10000/T;

  float R =(0.1/((5/V)-1));

  float T2 = 1.00/((1.00/298.15)+(1.00/3900.0)*log(R))-273.15;

  return T2;

  //return T1+273.15;
  
// return (1/T)+273.15;

  
}

boolean reached = false;

void testTemp(){

  //low speed t1 < t+20 & t2 <t+50 (typical +14 & +46)
  //med speed t1 < t+40 & t2 <t+60
  //hi  speed t1 < t+40 & t2 <t+60

  if(T1>90 || T2>90){
       errorHot++;
  }else if(T1<-20 || T2<-20 || temp<-20){
       errorCold++;
  }else if(T1>85 || T2>85 || T1 > (temp+40) || T2 > (temp+65)){
      if(mode!=mCoolDown || mode!=mTooHot){
        lastMode=mode;
        coolDown(0); x=9;
        mode=mTooHot;
      }
      errorHot=0; errorCold=0;    
  }else if(T1<-5 || T2<-5 || temp<-1){
      
      if(mode!=mStandby){
        lastMode=mode;
        
        coolDown(0); x=18;
        x=11;
        //mode==mTooCold;
      }
      
      errorHot=0; errorCold=0;      
  }else{
      errorHot=0;
      errorCold=0;
  }
  
}

void setLow(){
        displayOn(30000);
        digitalWrite(heater2, OFF);
        digitalWrite(heater1, ON);
        
        digitalWrite(fanM, OFF);
        digitalWrite(fanH, OFF);
        digitalWrite(fanL, ON);

        digitalWrite(wheel, ON);
        analogWrite(heatFan, 153);
      
        mode=mLow;
}

void setMed(){

        displayOn(30000);
        digitalWrite(heater1, ON);
        digitalWrite(heater2, ON);
        
        digitalWrite(fanH, OFF);
        digitalWrite(fanL, OFF);
        digitalWrite(fanM, ON);

        digitalWrite(wheel, ON);
        analogWrite(heatFan, 51);
      
        mode=mMed;
}

void setHigh(){

        displayOn(30000);
        digitalWrite(heater1, ON);
        digitalWrite(heater2, ON);

        digitalWrite(fanL, OFF);
        digitalWrite(fanM, OFF);
        digitalWrite(fanH, ON);

        digitalWrite(wheel, ON);
        analogWrite(heatFan, 51);
      
        mode=mHigh;
}

void setAuto(float humidity){
  
  if(humidity>70){
      setHigh();    
  }else if(humidity>60){
      setMed();    
  }else if(humidity>50){
      setLow();   
  }else if(mode!=mStandby && mode!=mSleeping && mode!=mSampling){
      coolDown(0);    
  }
  
}


void coolDown(int source){
        displayOn(10000);
        
        coolTime = millis();
        
        digitalWrite(heater1, OFF);
        digitalWrite(heater2, OFF);
        
        digitalWrite(fanH, OFF);        
        digitalWrite(fanM, OFF);
        digitalWrite(fanL, ON);

        digitalWrite(wheel, ON);
        analogWrite(heatFan, 153);
      
        mode=mCoolDown;

        if(source==1){
          offTime=3600000;
        }else{
          offTime=1800000;
        }
     
}



void turnOff(int source){

        displayOn(30000);
        digitalWrite(heater1, OFF);
        digitalWrite(heater2, OFF);
        
        digitalWrite(fanH, OFF);        
        digitalWrite(fanM, OFF);
        digitalWrite(fanL, OFF);

        digitalWrite(wheel, OFF);
        analogWrite(heatFan, 255);
      
        mode=mStandby;
        sinceOff=millis();

         if(source==1){
          
        }else{       
             offTime=1800000;
        }
}

void sample(){
        displayOn(30000);
        digitalWrite(heater1, OFF);
        digitalWrite(heater2, OFF);
        
        digitalWrite(fanH, OFF);        
        digitalWrite(fanM, OFF);
        digitalWrite(fanL, ON);

        digitalWrite(wheel, OFF);
        analogWrite(heatFan, 255);
      
        mode=mSampling;
        sampleStart=millis();
}

void setFanLow(){
        
        digitalWrite(heater1, OFF);
        digitalWrite(heater2, OFF);
        
        digitalWrite(fanH, OFF);        
        digitalWrite(fanM, OFF);
        digitalWrite(fanL, ON);

        digitalWrite(wheel, OFF);
        analogWrite(heatFan, 255);

        sinceFan=millis();
        mode=mFanLow;
}

void setFanMed(){

        digitalWrite(heater1, OFF);
        digitalWrite(heater2, OFF);
        
        digitalWrite(fanH, OFF);        
        digitalWrite(fanM, ON);
        digitalWrite(fanL, OFF);

        digitalWrite(wheel, OFF);
        analogWrite(heatFan, 255);

        sinceFan=millis();
        mode=mFanMed;
}

void setFanHigh(){

        digitalWrite(heater1, OFF);
        digitalWrite(heater2, OFF);
        
        digitalWrite(fanH, ON);        
        digitalWrite(fanM, OFF);
        digitalWrite(fanL, OFF);

        digitalWrite(wheel, OFF);
        analogWrite(heatFan, 255);

        sinceFan=millis();
        mode=mFanHigh;
}


void beep(){
    analogWrite(buzzer, 128);
    delay(50);
    analogWrite(buzzer, 0);
}

void sleep(){
    lastMode=mode;
    sleepTimer=0;
    coolDown(0); x=1;
  
}

void displayOn(unsigned long t){
    oledTime=t;
    oledTimer=millis();
  
}

int eep=0;

void setup() {
  // put your setup code here, to run once:

  pinMode(wheel, OUTPUT);
  pinMode(heatFan, OUTPUT);
  pinMode(fanL, OUTPUT);
  pinMode(fanM, OUTPUT);
  pinMode(fanH, OUTPUT);

  pinMode(heater1, OUTPUT);
  pinMode(heater2, OUTPUT);
 
  pinMode(buzzer, OUTPUT);
  pinMode(13, OUTPUT);
  
  pinMode(water, INPUT);
  
  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);
  pinMode(button3, INPUT_PULLUP);
  
  //pinMode(SCL, INPUT_PULLUP);
  //pinMode(SDA, INPUT_PULLUP);
  

  TCCR1B = TCCR1B & B11111000 | B00000101; //D9 and D10 pwm 30.64 Hz
  TCCR2B = TCCR2B & B11111000 | B00000010; // D3 and D11 pwm  3921.16 Hz

 // attachInterrupt(digitalPinToInterrupt(button1), button1Press, FALLING);
 // attachInterrupt(digitalPinToInterrupt(button2), button2Press, FALLING);

 digitalWrite(LED, HIGH);

  digitalWrite(wheel, OFF);
  digitalWrite(heatFan, OFF);
  digitalWrite(fanL, OFF);
  digitalWrite(fanM, OFF);
  digitalWrite(fanH, OFF);

  digitalWrite(heater1, OFF);
  digitalWrite(heater2, OFF);
  
  digitalWrite(buzzer, OFF);

  Serial.begin(9600);

  

  
 // Wire.begin();
//  Wire.setClock(100000);

  
  oled.begin(&Adafruit128x64, I2C_ADDRESS);

//  I2c.scan();
  //oled.set400kHz();  
  oled.setFont(ZevvPeep8x16);
  oled.setContrast(1);

  oledTimer=millis();
  
  hih.start();

  hih.update();
    
 hum = hih.humidity();
 temp = hih.temperature();


  beep();
  delay(250);
  beep();
  delay(250);
  beep();


  setAuto(hum);
    

  //Serial.println(hum);

  //wdt_enable(WDTO_8S);
digitalWrite(LED,LOW);
}



void loop() {


//   wdt_reset();
  

  lastMillis = millis(); 

  hih.update();
    
  hum = hih.humidity();
  temp = hih.temperature();

  
  if(temp<-20 || temp>60){
      errorSensor++;   
  }else{
      errorSensor=0; 
  }

  //Read internal temperature sensors
  adc=0;
  for(i=0; i<10; i++){
    adc += analogRead(therm1);
  }
  T1 = toTemp(adc);
  
  adc=0;
  for(i=0; i<10; i++){
    adc += analogRead(therm2);
  }
  T2 = toTemp(adc);

  //Handle serial input
  
   if (Serial.available() > 0) {
      // read the incoming byte:
      incomingByte = Serial.read();
  
  
  
      if(incomingByte=='S'){
        Serial.print(hum);
        Serial.print(" ");
        Serial.print(temp);
        Serial.print(" ");
        Serial.print(T1);
        Serial.print(" ");
        Serial.print(T2);
        Serial.print(" ");
        Serial.print(mode);
        Serial.print(" ");
        Serial.print(x);
        Serial.print(" ");
        Serial.print(buttonPressTime1);
        Serial.print(" ");
        Serial.print(buttonPressTime2);
        Serial.print(" ");
        Serial.println(buttonPressTime3);
      }else if(incomingByte=='B'){
        beep();
      }else if(incomingByte=='N'){
          //turn on
         setAuto(hum);
      }else if(incomingByte=='O'){
        //turn off
          if(mode==mLow || mode==mMed || mode==mHigh || mode==mFixedLow || mode==mFixedMed || mode==mFixedHigh){
              coolDown(1); x=10;
          }else if(mode==mFanLow || mode==mFanMed || mode==mFanHigh){     
               offTime=3600000;
               coolDown(0);
          }else  if(mode==mStandby){
              offTime=3600000;
              sinceOff=millis();
          }
      }else if(incomingByte=='F'){
          setFanMed();
      }else if(incomingByte=='H'){
          setHigh();
          mode = mFixedHigh;
      }else if(incomingByte=='M'){
          setMed();
          mode = mFixedMed;
      }else if(incomingByte=='L'){
          setLow();
          mode = mFixedLow;
      }
      
   }else{

   }

  //------------------------------Handle buttons
  
  if(buttonFlag1){

     displayOn(30000);
     if( buttonPressTime1>1001){
          //long press
          
          //beep(); delay(50); beep();

           if(mode==mLow || mode==mMed || mode==mHigh || mode==mFixedLow || mode==mFixedMed || mode==mFixedHigh){
              coolDown(0); x=22;          
          }else  if(mode==mStandby || mode==mCoolDown){
               setAuto(hum);
          }
          
     }else{
          //short press

          beep();
          
          if(mode==mLow || mode==mMed || mode==mHigh || mode==mFixedLow || mode==mFixedMed || mode==mFixedHigh){
              
              sleep();      x=8;   
          }else if(mode==mSleeping){
               setAuto(hum);         
          }else if(mode==mCoolDown){
              if(T1<(temp+20) && T2<(temp+60)){
                 setAuto(hum);          
              }
          }
          
     }

     
     buttonFlag1=0;
  }

  if(buttonFlag2){
    if(buttonPressTime2<1000){
      //mode
      displayOn(30000);
      if(mode==mLow || mode==mFixedLow){
          if(hum>50){
            beep();
            setMed();
          }else{
            beep();delay(50);beep();
          }
      }else if(mode==mMed || mode==mFixedMed){
          
          if(hum<55){
            beep();
            setLow();
          }else if(hum>65){
            beep();
            setHigh();
          }else{
            beep();delay(50);beep();
          }
       }else if(mode==mHigh || mode==mFixedHigh){
           if(hum<70){
              beep();
              setMed();
           }else{
              beep();delay(50);beep();            
           }
       }     
    }
     buttonFlag2=0;
  }

  
  if(buttonFlag3){

    displayOn(30000);
    
    beep();    
     
    buttonFlag3=0;
    
  }


  //--------------Check water full
   if(digitalRead(water) ){
      if(mode!=mStandby && mode!=mCoolDown){
        coolDown(0); x=3;
      }
      beep();
      delay(50);
   }

  //--------------Check temperature
  testTemp();

  //--------------Error loop
  if(errorHot>10 || errorCold>10 || errorSensor>10){

    coolDown(0);  x=4;

    oled.displayOn();
    oledOn=true;
  
    oled.home();    
    oled.set2X();
    oled.print("ERROR");
    oled.clearToEOL();
    oled.println();

    oled.set1X();

    oled.print(T1,0);
    //oled.print((char)247);
    oled.print("|C ");
  
    oled.print(T2,0);
    //oled.print((char)247); 
    oled.print("|C");

    oled.clearToEOL();
    oled.println();  

    if(errorHot>10){
        oled.print("Overheated");
    }else if(errorCold>10){
        oled.print("Too cold");
    }else if(errorSensor>10){
        oled.print("Sensor failed");
    }

    oled.clearToEOL();

    beep();
    
  }


  //--------------handle modes
  
  if(mode==mStandby){

      if((millis()-sinceOff)>offTime){
          sample();        
      }
     
    
  }else if(mode==mCoolDown){
      if((millis()-coolTime)>60000){
        if((T1<(temp+10) && T2<(temp+20)) || ((millis()-coolTime)>300000 && T1<(temp+20) && T2<(temp+30) )){
          if(sleepTimer==0){
            
             turnOff(1);
             //x=12;
             mode=mSleeping;        
          }else{
            
            turnOff(1);        
            //x=13;
          }
        }
      }else if((millis()-coolTime)>5000 && T1<(temp+10) && T2<(temp+20)){
        if(sleepTimer==0){
           
          turnOff(1);
           //x=14;
           mode=mSleeping;        
        }else{
          turnOff(1);
          //x=15;
        }
      }
    
  }else if(mode==mLow){
      if(hum>55 && temp<30){
          setMed();        
      }else if(hum<45){
          coolDown(0);     x=5;    
      }
    
  }else if(mode==mMed){
      if(hum<50){
          setLow();     
      }else if(hum>70  && temp<30){
          setHigh();        
      }
    
  }else if(mode==mHigh){
      if(hum<65){
          setMed();        
      }
    
  }else if(mode==mTooHot){
      if(T1<40 && T2<40 && temp<35){
          if(hum>55){
             setAuto(hum);          
          }else{
            coolDown(0); x=6;
          }
      }
    
  }else if(mode==mTooCold){
      if(T1>-5 && T2>-5 && temp>-1){          
          setMed();         
      }
    
  }else if(mode==mSleeping){
      sleepTimer++;

      if(sleepTimer>36000){
          if(lastMode==mLow|| lastMode==mMed|| lastMode==mHigh){
               setAuto(hum);         
          }else{
              
              turnOff(0);
              x=16;
          }
      
      }
    
  }else if(mode==mSampling){
      if((millis()-sampleStart)>60000){
          //sampling after 60s
          if(hum>55 && T1>0 && T2>0 && temp>0 && T1<50 && T2<60 && temp<35){
               setAuto(hum);            
          }else{
              
              turnOff(0);
              x=17;
          }
      }
  }else if(mode==mFanLow || mode==mFanMed || mode==mFanHigh){
      if((millis()-sinceFan)>7200000){
          
          turnOff(0);
      }
   }


 if(previousMode!=mode){
    updateOLED=true;  
 }

  if(oledOn){
      if(millis()-oledTimer>oledTime){
        oled.displayOff();
         oledOn=false;
      }
     
  }else{
       if(millis()-oledTimer<oledTime){
        oled.displayOn();
        oledOn=true;
        oledTimer=millis();
      }
      
       
  }

  
  
  //--------------Print output to OLED
  if(((millis()-oledMillis)>1000 || updateOLED) && oledOn){
      digitalWrite(LED, HIGH);
      oled.home(); 
      //LINE 1
      oled.set2X();
      oled.print(hum,0);
      oled.print("% ");
      oled.print(temp,0);
     // oled.print((char)247);
      oled.print("|C");
      
      oled.clearToEOL();      
      oled.println();
      //LINE 2
      oled.set1X();

      //oled.print(buttonPressTime1);
      //oled.print(" ");

      //oled.print(x);
      //    oled.print(" ");

      //oled.print(buttonPressTime1);
       //oled.print(" ");
         //oled.print(buttonPressTime2);
          // oled.print(" ");
      
      oled.print(T1,0);
      //oled.print((char)247);
      oled.print("|C ");
    
      oled.print(T2,0);
      //oled.print((char)247); 
      oled.print("|C");

      oled.clearToEOL();
      oled.println();

      //LINE 3


    //oled.print(lastISR/1000);
    //oled.print(" ");
    //oled.print(millis()/1000);

    
     if(digitalRead(water)){
        oled.print("Water full ");
     }else if(mode==mStandby){
        oled.print("Standby "); 
        float sleepSecs = (offTime/1000)-(millis() - sinceOff)/1000;

        float mins = floor(sleepSecs/60);

        float secs = sleepSecs-(60*mins);

        oled.print(mins,0);
        oled.print(":");
        if (secs<10) oled.print("0");   
        oled.print(secs,0);  
        oled.print(" "); 
        oled.print(x); 
     }else if(mode==mCoolDown){
        oled.print("Cooling ");  
        oled.print(x);    
     }else if(mode==mLow){
        oled.print("Low");      
     }else if(mode==mMed){
        oled.print("Med");      
     }else if(mode==mHigh){
        oled.print("High");    
     }else if(mode==mFixedLow){
        oled.print("Fixed Low");      
     }else if(mode==mFixedMed){
        oled.print("Fixed Med");      
     }else if(mode==mFixedHigh){
        oled.print("Fixed High");      
     }else if(mode==mTooHot){
        oled.print("Too hot");
     }else if(mode==mTooCold){
        oled.print("Too cold");
     }else if(mode==mSleeping){
        oled.print("Sleeping ");
        float sleepSecs = 3600.1-(sleepTimer/10);

        float mins = floor(sleepSecs/60);

        float secs = sleepSecs-(60*mins);

        oled.print(mins,0);
        oled.print(":");
        if (secs<10) oled.print("0");
        oled.print(secs,0);
        oled.print(" "); 
        oled.print(x); 
     }else if(mode==mSampling){
        oled.print("Sampling ");
        float sleepSecs = 60-(millis() - sampleStart)/1000;

        float mins = floor(sleepSecs/60);

        float secs = sleepSecs-(60*mins);

        oled.print(mins,0);
        oled.print(":");
        if (secs<10) oled.print("0");   
        oled.print(secs,0);  
    }else if(mode==mFanLow || mode==mFanMed || mode==mFanHigh){
        oled.print("Fan mode "); 
        float fanSecs = 7200-(millis() - sinceFan)/1000;

        float mins = floor(fanSecs/60);

        float secs = fanSecs-(60*mins);

        oled.print(mins,0);
        oled.print(":");
        if (secs<10) oled.print("0");   
        oled.print(secs,0);  
        oled.print(" "); 
        oled.print(x);    
     }else{
        oled.print("Error");      
     }
           
           
      oled.clearToEOL();

      oledMillis = millis();
      updateOLED=false;
      digitalWrite(LED, LOW);
  }

  previousMode=mode;

 
  


  //--------------Wait for buttons or 100ms    
  while((millis()-lastMillis)<100){
              

      if(!digitalRead(button1)){
        //button 1 pressed
      
        startMillis = millis();

        buttonPressTime1=0;
        
        
          while(!digitalRead(button1)){
              buttonPressTime1 = millis() - startMillis;  
              if( buttonPressTime1>1000){
                  //long press
                  beep(); delay(50); beep();
                  while(!digitalRead(button1)){
                      buttonPressTime1 = millis() - startMillis;   
                  }
                  break;
              }
          }
  
         
  
          if(buttonPressTime1>50){
            buttonFlag1=1;
           // lastISR=millis();
          }
        
      }


      if(!digitalRead(button2)){
        //button 2 pressed
      
        startMillis = millis();

        buttonPressTime1=2;
      
        while(!digitalRead(button2)){           
            buttonPressTime2 = millis() - startMillis;  
            if( buttonPressTime2>1000){
                //long press
                //beep(); delay(50); beep();
                while(!digitalRead(button2)){
                  buttonPressTime2 = millis() - startMillis;    }
                break;
            }
        }


        if(buttonPressTime2>50){
          buttonFlag2=1;
         // lastISR=millis();
        }
      }

       if(!digitalRead(button3)){
        //button 3 pressed
      
        startMillis = millis();

        buttonPressTime3=0;
      
        while(!digitalRead(button3)){           
            buttonPressTime3 = millis() - startMillis;  
            
        }


        if(buttonPressTime3>50){
          buttonFlag3=1;
         // lastISR=millis();
        }
      }


  }

  
 
}
