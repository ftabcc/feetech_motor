#ifndef _SCSERIAL_H
#define _SCSERIAL_H

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "SCS.h"

class SCSerial : public SCS
{
public:
	SCSerial();
	SCSerial(u8 End);
	SCSerial(u8 End, u8 Level);
protected:
	int writeSCS(unsigned char *nDat, int nLen);// nLen바이트 출력
	int readSCS(unsigned char *nDat, int nLen);// nLen바이트 입력
	int readSCS(unsigned char *nDat, int nLen, unsigned long TimeOut);// 타임아웃을 지정하여 nLen바이트 입력
	int writeSCS(unsigned char bDat);// 1바이트 출력
	void rFlushSCS();// 수신 버퍼 비우기
	void wFlushSCS();// 송신 버퍼 비우기
public:
	unsigned long IOTimeOut;// 입출력 타임아웃
	HardwareSerial *pSerial;// 시리얼 포트 포인터
};


#endif