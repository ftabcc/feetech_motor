
/*
모든 서보의 피드백 파라미터를 읽습니다:
위치, 속도, 부하, 전압, 온도, 이동 상태, 전류.

FeedBack 함수는 서보의 파라미터를 버퍼에 읽어 저장합니다.
Readxxx(-1) 함수는 버퍼에 저장된 해당 서보의 상태를 반환합니다.

Readxxx(ID) 함수에서
ID = -1이면 FeedBack 버퍼의 값을 반환합니다.
ID >= 0이면 읽기 명령을 통해 지정한 ID의 서보 상태를 직접 반환하므로
FeedBack 함수를 호출할 필요가 없습니다.
*/

#include <SCServo.h>

SMS_STS sms_sts;
int LEDpin = 13;

void setup()
{
  pinMode(LEDpin,OUTPUT);
  digitalWrite(LEDpin, HIGH);
  //Serial1.begin(115200);//sms舵机波特率115200
  Serial1.begin(1000000);//sts舵机波特率1000000
  Serial.begin(115200);
  sms_sts.pSerial = &Serial1;
  delay(1000);
}

void loop()
{
  int Pos;
  int Speed;
  int Load;
  int Voltage;
  int Temper;
  int Move;
  int Current;
  sms_sts.FeedBack(1);
  if(!sms_sts.getLastError()){
    digitalWrite(LEDpin, LOW);
    Pos = sms_sts.ReadPos(-1);
    Speed = sms_sts.ReadSpeed(-1);
    Load = sms_sts.ReadLoad(-1);
    Voltage = sms_sts.ReadVoltage(-1);
    Temper = sms_sts.ReadTemper(-1);
    Move = sms_sts.ReadMove(-1);
    Current = sms_sts.ReadCurrent(-1);
    Serial.print("Position:");
    Serial.println(Pos);
    Serial.print("Speed:");
    Serial.println(Speed);
    Serial.print("Load:");
    Serial.println(Load);
    Serial.print("Voltage:");
    Serial.println(Voltage);
    Serial.print("Temper:");
    Serial.println(Temper);
    Serial.print("Move:");
    Serial.println(Move);
    Serial.print("Current:");
    Serial.println(Current);
    delay(10);
  }else{
    digitalWrite(LEDpin, HIGH);
    Serial.println("FeedBack err");
    delay(500);
  }
  
  Pos = sms_sts.ReadPos(1);
  if(!sms_sts.getLastError()){
    digitalWrite(LEDpin, LOW);
    Serial.print("Servo position:");
    Serial.println(Pos, DEC);
    delay(10);
  }else{
    Serial.println("read position err");
    digitalWrite(LEDpin, HIGH);
    delay(500);
  }
  
  Voltage = sms_sts.ReadVoltage(1);
  if(!sms_sts.getLastError()){
    digitalWrite(LEDpin, LOW);
    Serial.print("Servo Voltage:");
    Serial.println(Voltage, DEC);
    delay(10);
  }else{
    Serial.println("read Voltage err");
    digitalWrite(LEDpin, HIGH);
    delay(500);
  }
  
  Temper = sms_sts.ReadTemper(1);
  if(!sms_sts.getLastError()){
    digitalWrite(LEDpin, LOW);
    Serial.print("Servo temperature:");
    Serial.println(Temper, DEC);
    delay(10);
  }else{
    Serial.println("read temperature err");
    digitalWrite(LEDpin, HIGH);
    delay(500);    
  }

  Speed = sms_sts.ReadSpeed(1);
  if(!sms_sts.getLastError()){
    digitalWrite(LEDpin, LOW);
    Serial.print("Servo Speed:");
    Serial.println(Speed, DEC);
    delay(10);
  }else{
    Serial.println("read Speed err");
    digitalWrite(LEDpin, HIGH);
    delay(500);    
  }
  
  Load = sms_sts.ReadLoad(1);
  if(!sms_sts.getLastError()){
    digitalWrite(LEDpin, LOW);
    Serial.print("Servo Load:");
    Serial.println(Load, DEC);
    delay(10);
  }else{
    Serial.println("read Load err");
    digitalWrite(LEDpin, HIGH);
    delay(500);    
  }
  
  Current = sms_sts.ReadCurrent(1);
  if(!sms_sts.getLastError()){
    digitalWrite(LEDpin, LOW);
    Serial.print("Servo Current:");
    Serial.println(Current, DEC);
    delay(10);
  }else{
    Serial.println("read Current err");
    digitalWrite(LEDpin, HIGH);
    delay(500);    
  }

  Move = sms_sts.ReadMove(1);
  if(!sms_sts.getLastError()){
    digitalWrite(LEDpin, LOW);
    Serial.print("Servo Move:");
    Serial.println(Move, DEC);
    delay(10);
  }else{
    Serial.println("read Move err");
    digitalWrite(LEDpin, HIGH);
    delay(500);    
  }
  Serial.println();
}
