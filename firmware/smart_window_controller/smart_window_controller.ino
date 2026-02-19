#include <Stepper.h>
#include <pitches.h>
#include <FreqPeriodCounter.h>
#include <LiquidCrystal.h>

byte user1[8] = {  // 섭씨 기호
  B00011,
  B00011,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

const int stepsPerRevolution = 64;  // 모터 한 회전당 스텝 수
Stepper myStepper(stepsPerRevolution, 11, 9, 10, 8); 
int trigPin = 5; // 초음파 센서 trig
int echoPin = 4; // 초음파 센서 echo
int waterPin = 54; // 수위 센서
int waterValue = 0;
int speakerPin = 55; // 스피커
int melody[] = { // 경보음
  NOTE_G4, NOTE_G4, NOTE_G4
};
int noteDuration = 4; // 4분 음표

// 핀 번호 (RS, E, DB4, DB5, DB6, DB7)
LiquidCrystal lcd(44, 45, 46, 47, 48, 49); // LCD 연결

// 풍속 센서
const byte counterPin = 3; 
const byte counterInterrupt = 1; // = pin 3
FreqPeriodCounter counter(counterPin, micros, 0);
unsigned long windspeed; 

// 풀다운 버튼
int pin_button = 14;
boolean state_previous = false;
boolean state_current;

int window_state = 1; // 창문닫힘 : 0, 창문열림 : 1

void setup() {
  Serial.begin(9600);
  myStepper.setSpeed(300); // 스텝 모터 속도
  pinMode(trigPin, OUTPUT); // 초음파 센서
  pinMode(echoPin, INPUT);
  pinMode(56, INPUT); // 온도 센서
  attachInterrupt(counterInterrupt, counterISR, CHANGE); // 풍속센서
  Serial1.begin(9600); // Serial1을 사용하여 블루투스 모듈 초기화 RX, TX = 18, 19
  lcd.begin(16, 2); // LCD 초기화
}

void open_window() {
  if (window_state == 0) {
    lcd.clear(); // LCD 화면 지우기
    lcd.print("open!"); // 문자열 출력
    for(int i=0; i<28; i++) {  //  64 * 28
      myStepper.step(-stepsPerRevolution); //시계 방향
    }
    window_state = 1;
    lcd.clear(); // LCD 화면 지우기
  }
  delay(500);
}

void close_window() {
  if (window_state == 1) {
    lcd.clear(); // LCD 화면 지우기
    lcd.print("close!"); // 문자열 출력
    for(int i=0; i<28; i++) {  // 64 * 28
      myStepper.step(stepsPerRevolution); // 반시계 방향
    }
    window_state = 0;
    lcd.clear(); // LCD 화면 지우기
  }
  delay(500);
}

void loop() {
   // 풀다운 버튼
  state_current = digitalRead(pin_button);
  
  if (state_current) { // Button is pressed
    if (state_previous == false) { // Compares to previous state
      state_previous = true;
      
      // 회전 방향에 따라 회전
      if (window_state==0) {
        open_window();
      } else {
        close_window();
      }
    }
    delay(50); // Debouncing
  } else {
    state_previous = false;
  }

  // 초음파 센서
  digitalWrite(trigPin, HIGH);
  delay(100);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  float duration = pulseIn(echoPin, HIGH);
  float distance = duration * 340 / 10000 / 2;
  Serial.println(String("초음파 : ") + distance);
  if (distance <= 10) {
    if (window_state == 1) {
      for (int thisNote = 0; thisNote < sizeof(melody) / sizeof(int); thisNote++) { // 경보음
        // 음표 길이를 시간으로 변환
        int noteLength = 1000 / noteDuration;
        // 단음 재생
        tone(speakerPin, melody[thisNote], noteLength);
        delay(noteLength);
        noTone(speakerPin); // 현재 음 재생 중지
      }
      close_window();
    }
  }

  // 수위센서
  waterValue = analogRead(waterPin);
  Serial.println(String("수위 : ") + waterValue);
  if (waterValue >= 100) {
    close_window();
  }

  // 풍속센서
  if(counter.ready()) { 
    windspeed = counter.hertz(); // 주파수를 헤르츠 단위로 반
    Serial.println(String("풍속 : ") + windspeed);
    if (windspeed >= 8) {  // 풍속 8m/s 이상일 경우
      close_window();
    }
  } else {
    windspeed = 0;
    Serial.println(String("풍속 : ") + windspeed);
  }

  // 블루투스
  if (Serial1.available()) {
    // 블루투스로부터 데이터 읽기
    char data = Serial1.read();
    Serial.println(data);
    if (data == 'O') {
      open_window();
    }
    if (data == 'C') {
      close_window();
    }
  }

  // 온도 LCD 출력
  int reading = analogRead(56);
  // ADC 반환값을 전압으로 변환
  float voltage = reading * 5.0 / 1024.0;
  // 전압에 100을 곱하면 섭씨 온도를 얻을 수 있다.
  float temp_C = voltage * 100;
  lcd.clear();
  lcd.print("Temp: ");
  lcd.print(temp_C);
  lcd.write(byte(0));
  lcd.print("C");

  // 풍속 LCD 출력
  lcd.setCursor(0, 1);
  lcd.print("Wind: ");
  lcd.print(windspeed);
  lcd.print("m/s");

  delay(1000);
}

void counterISR() {
  counter.poll();
} // 인터럽트 서비스 루틴 (ISR)
// 인터럽트 : 일반적인 프로그램 흐름을 중단하고 특정한 작업을 수행한 다음, 다시 원래의 작업으로
