#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address 0x27, 16 column and 2 rows

long timemarkOxyStartOff = 0; // thời gian trước đó của Oxy
long OxyTankStartRunTime = 0; // mốc thời gian thùng Oxy bắt đầychạy
long OxyTankRunningTime = 0; // thời gian thùng oxy chạy
long timemarkStartMixLime = 0; //thời gian trước đó của voi
long lastOxyTime = 0; //thời gian bắt đầu mỗi lần oxy chạy
long currentTimeWhistleOn = 0;
long lastTimeWhistleOn = 0;
int totalOxyRunningTime = 0; // tổng thời gian Oxy chạy
int counterBuoyLimeOnEachCycle = 0; // đếm số lần phao vôi giật trong từng chu kì
int counterLimeTank = 0; // đếm tổng số lần ngoáy vôi
int numberInCycle = 3; //số lần giật giao trong một chu kỳ 
int cycle1 = 0; // chu kỳ có 1 lần giật 
int cycle3 = 0; // chu kỳ có 3 lần giật 
int valveLockTime = 0; // thời gian khóa van sau khi bình OXY tắt
bool lastLimeBuoyState = 1; // trạng thái trước đó của phao vôi
bool limeBuoyState = 0; // trạng thái của phao vôi
bool oxyBuoyState = 0; // trạng thái của phao Oxy
bool lastOxyBuoyState = 1; // trạng thái trước đó của phao Oxy
bool LcdState = 1; //biến chuyển đổi trạng thái bật tắt LCD
bool previousStateLcdButton = 1; // trạng thái trước của nút bấm bật tắt LCD
bool isThreadCaseHappen = 0; // trường hợp xảy ra khi vôi chạy trước 
bool isStartMixLime = 0; //biến trung gian để bật tắt ngoáy vôi
bool isOxyBuoyOff = 0; 
bool isOxyBuoyOn = 0;
volatile bool isWhistleOn = 0;
volatile bool isLcdOn = 0; // biến kích hoạt trình con xử lý ngắt ISR của màn hình LCD
volatile bool isResetOxyTimer = 0; // biến kích hoạt trình con xử lý ngắt ISR: reset thời gian Oxy bắt đầu chạy

void setup() {
  Serial.begin(9600);
  pinMode(2, INPUT_PULLUP); // nút bật tắt LCD
  pinMode(3, INPUT_PULLUP); // nút bật tắt còi
  pinMode(4, INPUT_PULLUP); // cảm biến thùng vôi
  pinMode(5, INPUT_PULLUP); // cảm biến thùng Oxy
  pinMode(6, OUTPUT); // máy bơm ngoáy vôi
  pinMode(7, OUTPUT); // van từ
  pinMode(8, OUTPUT); // còi
  lcd.begin();
  lcd.backlight();
  attachInterrupt(digitalPinToInterrupt(2), LCD_ISR, CHANGE); 
  attachInterrupt(digitalPinToInterrupt(3), reset_ISR, CHANGE); 
}


void LCD_display(){

  lcd.setCursor(0, 0);         
  lcd.print("3");        
  lcd.setCursor(0, 1);         
  lcd.print(cycle3); 

  lcd.setCursor(2, 0);        
  lcd.print("1");        
  lcd.setCursor(2, 1);         
  lcd.print(cycle1); 

  lcd.setCursor(4, 0);         
  lcd.print("OXI");        
  lcd.setCursor(4, 1);        
  lcd.print(totalOxyRunningTime);

  lcd.setCursor(8, 0);          
  lcd.print("Voi");        
  lcd.setCursor(8, 1);        
  lcd.print(counterLimeTank);

  lcd.setCursor(12, 0);
  lcd.print("Time");
  lcd.setCursor(12, 1);        
  lcd.print(valveLockTime);
}

void LCD_ISR(){
  isLcdOn = 1;
}

void display_LCD(){
if(isLcdOn == 1){ // xảy ra 
  if(digitalRead(2) == LOW && previousStateLcdButton == HIGH) // khi bấm nút 
      {
        LcdState =! LcdState; // nếu LCD đang tắt thì bật và ngược lại
        if(LcdState == LOW){
          lcd.noBacklight();
          lcd.noDisplay();
        }else{
          lcd.backlight();
          lcd.display();
        }
        previousStateLcdButton = LOW; // cật nhật trạng thái của nút bấm
      }
      else if(digitalRead(2) == HIGH && previousStateLcdButton == LOW) // khi nhả nút 
      {
        previousStateLcdButton = HIGH; // cật nhật trạng thái của nút bấm
      }
      isLcdOn = 0; // đưa trạng thái ISR về 0
  }
}



void lock_valve(){ // khóa van từ
  limeBuoyState = digitalRead(4);
  if(limeBuoyState == LOW){ // nếu phao vôi đang bật thì khóa van từ
    digitalWrite(7, LOW);
  }
}

void pump_lime(){ // bơm vôi 
  oxyBuoyState = digitalRead(5);
  if(oxyBuoyState != lastOxyBuoyState){ 
    lastOxyBuoyState = oxyBuoyState;
    if(oxyBuoyState == LOW){ // nếu phao oxy bật
        OxyTankStartRunTime = millis(); // gán thời gian Oxy bắt đầu chạy
        lastOxyTime = 0;
        if(digitalRead(4)== LOW || digitalRead(6) == HIGH){ // trong trường hợp phao vôi(4) đang bật hoặc động cơ ngoáy vôi (6) đang chạy
          isThreadCaseHappen = 1; // sinh ra trường hợp đặc biệt
        }else if(digitalRead(4)==HIGH){ // trong trường hợp chỉ có phao Oxy giật 
          digitalWrite(7,HIGH); // mở van từ
        }  
      }else{ // trong trường hợp phao Oxy tắt thì 10 phút sau van từ mới được khóa
        isOxyBuoyOff = 1; // phao oxy đã tắt
        timemarkOxyStartOff = millis(); // gán thời gian lúc phao Oxy bắt đầu ngắt
        
      }
      delay(50);
  }else{
    if(oxyBuoyState == LOW && digitalRead(6) == LOW && digitalRead(4) == HIGH){ // nếu phao Oxy vẫn giữ bật + (phao vôi và động cơ ngoáy tắt)
      digitalWrite(7, HIGH); // Van từ vẫn được chạy
    }
    delay(50);
  }
}


// hàm này có tác dụng để chạy trường hợp xảy ra khi phao vôi hay động cơ ngoáy chạy. Phải đợi khi 2 thứ trên chạy xong thì
// van từ mới được mở nếu không khi bơm nước xuống, nước bể sẽ bị đục
void happen_thread_case(){
  if(isThreadCaseHappen == 1){
    if(digitalRead(4)== HIGH && digitalRead(6) == LOW){ // cho đến khi phao vôi(4) và động cơ(6) tắt
              digitalWrite(7,HIGH); // van từ mở
              isThreadCaseHappen = 0; 
            }
  }          
          
}

//hàm này có tác dụng đếm thời gian mỗi khi thùng oxy đc bơm nước, phao 5 có tín hiệu. Một biến đo thời gian Oxy: OxyTankRunningTime
//nếu nó tăng 1 đơn vị thì biến đếm tổng thời gian Oxy cx chạy tăng 1.
void count_oxy_run_time(){
  if(digitalRead(5) == LOW){
      OxyTankRunningTime = (millis() - OxyTankStartRunTime)/60000; // đếm thời gian thùng Oxy bắt đầu bơm tính theo phút
  }
  if(lastOxyTime + 1 == OxyTankRunningTime){
    lastOxyTime = OxyTankRunningTime;
    ++totalOxyRunningTime;
    if(totalOxyRunningTime == 360){
      isWhistleOn = 1;
      lastTimeWhistleOn = millis();
    }
  }
}

void delay_vale(){
    if(isOxyBuoyOff == 1){ // nếu phao Oxy tắt
      if((millis() - timemarkOxyStartOff) >= 600000){ // nếu thời gian >= 10 phút
        digitalWrite(7, LOW); // khóa van từ
        valveLockTime = 0; // đưa thời gian delay khóa van từ về 0 -> hiện ở màn hình
        isOxyBuoyOff = 0; 
      }else if(digitalRead(4) == LOW){ // nếu trong quá trình đợi 10 phút mà phao vôi bật thì cũng tắt van từ luôn
        digitalWrite(7, LOW);
        valveLockTime = 0;
        isOxyBuoyOff = 0;
      }else{
        valveLockTime = (millis() - timemarkOxyStartOff) / 600b00; // đưa thời gian khoá van từ ra màn hình LCD theo phut
      }
  }
  
}


void mix_lime(){ // ngoáy vôi
  if(counterBuoyLimeOnEachCycle == numberInCycle){ // khi số lần phao giật bằng số lần ngoáy vôi trong một chu kì 
    switch(counterBuoyLimeOnEachCycle){
      case 1: ++cycle1; break; // tăng số lần chu kì 1 để hiện lên màn hình
      case 3: ++cycle3; break; // tăng số lần chu kì 3 để hiện lên màn hình
    }
    isStartMixLime = 1; // bắt đầu ngoáy vôi
    timemarkStartMixLime = millis(); // gán mốc thời gian bắt đầu ngoáy vôi
    counterBuoyLimeOnEachCycle = 0; // đưa biến đếm số lần phao vôi giật về 0
    numberInCycle-=2; // giảm số lần ngoáy trong 1 chu kỳ
  }
  if(numberInCycle <= 0){ 
    numberInCycle = 1;
  }
}

void start_mix_lime(){ // bắt đầu thực hiện ngoáy vôi
  if(isStartMixLime == 1){
    if((millis() - timemarkStartMixLime) <= 60000){ // ngoáy vôi trong thời gian 1 phút
      digitalWrite(6, HIGH);
    }else{
      digitalWrite(6,LOW);
      isStartMixLime = 0;
    }
  }
}

void count_lime_buoy(){ // cảm biến và đếm số lần phao vôi giật
  limeBuoyState = digitalRead(4);
  if(limeBuoyState != lastLimeBuoyState){
    lastLimeBuoyState = limeBuoyState;
    if (limeBuoyState == LOW && digitalRead(6) == LOW) {
         counterBuoyLimeOnEachCycle++; // đếm số lần phao giật trong từng chu kì 
         ++counterLimeTank; // đếm tổng số thùng vôi 
      }else if(limeBuoyState == HIGH){ // cho đến khi phao vôi ngắt (tức là thùng đầy) thì mới ngoáy vôi
        mix_lime();
      }
      delay(50);
  }
  
}


void reset_ISR(){
  isResetOxyTimer = 1;
  isWhistleOn = 0;
  totalOxyRunningTime = 0; // đưa thời gian Oxy bắt đầu chạy về 0
}

void clear_LCD(){
  if(isResetOxyTimer == 1){
    lcd.clear(); // clear màn hình LCD vì khi currentTimeOxy có 2 hoặc 3 chữ số thì khi đưa về 0, chữ số đứng thứ 2 3 ko mất 
                  // ví dụ: currentTimeOxy đang là 123 thì khi đưa currentTimeOxy = 0, LCD sẽ hiển thị 023 chứ ko phải là 0 
    isResetOxyTimer = 0;
  }
}



void run_whistle(){
  if(isWhistleOn == 1){
    currentTimeWhistleOn = millis();
    if(currentTimeWhistleOn - lastTimeWhistleOn < 60000){
      digitalWrite(8, HIGH);
    }else{
      digitalWrite(8, LOW);
    }
    if(currentTimeWhistleOn - lastTimeWhistleOn > 180000){
      lastTimeWhistleOn = currentTimeWhistleOn;
    }
  }else{
    digitalWrite(8, LOW);
  }
  
}

void loop() {
  // put your main code here, to run repeatedly:
  lock_valve();
  count_lime_buoy();
  start_mix_lime();
  pump_lime();
  count_oxy_run_time();
  happen_thread_case();
  delay_vale();
  LCD_display();
  display_LCD();
  run_whistle();
  clear_LCD();
}
