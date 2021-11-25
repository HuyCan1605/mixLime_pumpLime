#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address 0x27, 16 column and 2 rows

int count = 0;
int count_Oxy = 0;
int count_voi = 0;
int lastStatus = 1; 
int sensorVal = 0;
int number_of_touch = 4;
int sensorVal2 = 0;
int lastStatus2 = 1;
int sensorVal3 = 0; //nút bấm bật tắt LCD
int lastStatus3 = 1;
int cycle1 = 0;
int cycle2 = 0;
int cycle3 = 0;
int cycle4 = 0;
int numberPress = 0;
int whistle = 0;
int sensorVal4 = 0;
int lastStatus4 = 1;
unsigned long currentTimeOn = 0;
unsigned long lastTimeOn = 0;

void setup() {
  //start serial connection
  Serial.begin(9600);
  //configure pin 2 as an input and enable the internal pull-up resistor
  pinMode(4, INPUT_PULLUP);
//  pinMode(6, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);
  pinMode(13, INPUT_PULLUP);
  pinMode(6, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(12, OUTPUT);
 
  lcd.begin();
  lcd.backlight();
}

void restartCycle(){
  
  if(number_of_touch <= 0){
    number_of_touch = 1;
  }
}



void ngoayVoi(){
  
  if(count == number_of_touch){
    switch(count){
      case 1: cycle1++; break;
      case 2: cycle2++; break;
      case 4: cycle4++; break;
    }
    digitalWrite(6, HIGH);
    delay(6000);
    digitalWrite(6, LOW);
    count = 0;
    number_of_touch-=2;
  }
  restartCycle();
}

void turnOn(){
  if(count_voi % 6 == 0 ){
//    Serial.println("turnOn");
      whistle = 1;
  }
}


void runWhistle(){

  if(whistle == 1){
//    Serial.println("runWhistle");
    currentTimeOn = millis();
    if(currentTimeOn - lastTimeOn < 2000){
      digitalWrite(12, HIGH);
    }else{
      digitalWrite(12, LOW);
    }
    if(currentTimeOn - lastTimeOn > 3000){
      lastTimeOn = currentTimeOn;
    }
  }
  
}


void turnOff(){
    if(whistle == 1){
//      Serial.println("turnOn");
      sensorVal4 = digitalRead(13);
      if(sensorVal4 != lastStatus4){
        lastStatus4 = sensorVal4;
      if(sensorVal4 == LOW){
      digitalWrite(12, LOW);
      whistle = 0;
      }
      delay(50);
      }
    }   
}

void demNgoayVoi(){
  sensorVal = digitalRead(4);
  if(sensorVal != lastStatus){
    lastStatus = sensorVal;
    if (sensorVal == LOW) {
         count++;
         Serial.println(count);
         count_voi++;
         turnOn();
         
         LCD_off_on();
         runWhistle();
         
      }else{
        ngoayVoi();
      }
      delay(200);
  }
  
}

void tatVan(){ // khóa van từ
  sensorVal = digitalRead(2);
  if(sensorVal == LOW){
    digitalWrite(8, LOW);
  }
}

void Delay(long time){
  unsigned long currentTime = (unsigned long)millis();
  unsigned long previousTime = currentTime;
  while((unsigned long)(currentTime - previousTime) <= time){
    currentTime = (unsigned long)millis();
    sensorVal = digitalRead(2);
    if(sensorVal == LOW){
      digitalWrite(8, LOW);
      break;
    }
    if(currentTime < previousTime){
      previousTime = currentTime;
    }
  }
}



void hutVoi(){
  sensorVal2 = digitalRead(6);
  if(sensorVal2 != lastStatus2){
    lastStatus2 = sensorVal2;
    if(sensorVal2 == LOW){
        tatVan();
        sensorVal = digitalRead(2);
        switch(sensorVal){
      case 1: // trường hợp phao vôi không giật 
          digitalWrite(8, HIGH); // van từ mở
          count_Oxy++; // đếm phao oxy chạy tăng
      break;
      
      case 0: // trường hợp phao vôi giật -> bình vôi hết nước
      while(true){ // kiểm tra cho đến khi vôi đầy thì mới được bơm nước
        LCD_off_on();
        runWhistle();
        sensorVal = digitalRead(2);
          if(sensorVal == HIGH){
                break;
          }
      }
        digitalWrite(8, HIGH); //khi vôi tắt thì van từ mở 
        count_Oxy++; // đếm số lần phao oxy chạy. Sẽ chỉ chạy 1 lần nếu xuất hiện ấn giữ
      break;
        }
      }else{
        Delay(900000);
        digitalWrite(8, LOW);
      }
      delay(50);
  }else{
    if(sensorVal2 == LOW){
      digitalWrite(8, HIGH);
    }
    delay(50);
  }

  
}

void LCD_display(){

  lcd.setCursor(0, 0);         
  lcd.print("4");        
  lcd.setCursor(0, 1);         
  lcd.print(cycle4); 

  lcd.setCursor(2, 0);         
  lcd.print("2");        
  lcd.setCursor(2, 1);         
  lcd.print(cycle2); 

  lcd.setCursor(4, 0);        
  lcd.print("1");        
  lcd.setCursor(4, 1);         
  lcd.print(cycle1); 

  lcd.setCursor(6, 0);         
  lcd.print("OXI");        
  lcd.setCursor(6, 1);        
  lcd.print(count_Oxy);

  lcd.setCursor(10, 0);         
  lcd.print("Voi");        
  lcd.setCursor(10, 1);        
  lcd.print(count_voi);

}

void LCD_off_on(){
  int sensorVal3 = digitalRead(10);
  if(sensorVal3 != lastStatus3){
    lastStatus3 = sensorVal3;
    if(sensorVal3 == LOW){
      numberPress++;
    }
  }
  if(numberPress%2 != 0){
    lcd.noBacklight();
    lcd.noDisplay();
  }else{
    lcd.backlight();
    lcd.display();
    LCD_display();
  }
}
void loop() {
  //read the pushbutton value into a variable
  
  tatVan();
  
  demNgoayVoi();
  
  hutVoi();

  LCD_off_on();

  turnOff();
  runWhistle();
  
}
