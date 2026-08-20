/*
동기 읽기(Synchronous read) 명령: 서보 ID1과 ID2의 위치 및 속도 정보를 읽어옵니다.
*/

#include <SCServo.h>

SMS_STS sms_sts;

uint8_t ID[] = {1, 2};
uint8_t rxPacket[4];
int16_t Position;
int16_t Speed;

void setup()
{
  Serial.begin(115200);
  //Serial1.begin(115200);//sms舵机波特率115200
  Serial1.begin(1000000);//sts舵机波特率1000000
  sms_sts.pSerial = &Serial1;
  sms_sts.syncReadBegin(sizeof(ID), sizeof(rxPacket), 5);//10*10*2=200us<5ms
  delay(1000);
}

void loop()
{  
  sms_sts.syncReadPacketTx(ID, sizeof(ID), SMS_STS_PRESENT_POSITION_L, sizeof(rxPacket));//syncread & receive packet.
  for(uint8_t i=0; i<sizeof(ID); i++){
    if(!sms_sts.syncReadPacketRx(ID[i], rxPacket)){
     Serial.print("ID:");
     Serial.println(ID[i]);
     Serial.println("sync read error!");
     continue;
    }
    Position = sms_sts.syncReadRxPacketToWrod(15);//2바이트 디코딩: 15번째 비트는 방향 비트이며, 매개변수 값이 0이면 방향 비트가 없음을 나타냅니다.
    Speed = sms_sts.syncReadRxPacketToWrod(15);//2바이트 디코딩: 15번째 비트는 방향 비트이며, 매개변수 값이 0이면 방향 비트가 없음을 나타냅니다.
    Serial.print("ID:");
    Serial.println(ID[i]);
    Serial.print("Position:");
    Serial.println(Position);
    Serial.print("Speed:");
    Serial.println(Speed);
  }
  delay(10);
}
