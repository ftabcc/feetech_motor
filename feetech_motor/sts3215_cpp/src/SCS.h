#ifndef _ESP_SYSTEM
#define _ESP_SYSTEM

#include "INST.h"

class ESP{
public:
	SCS();
	SCS(u8 End);
	SCS(u8 End, u8 Level);

	int genWrite(u8 ID, u8 MemAddr, u8 *nDat, u8 nLen);//일반 쓰기 명령
	int regWrite(u8 ID, u8 MemAddr, u8 *nDat, u8 nLen);//비동기 쓰기 명령
	int RegWriteAction(u8 ID = 0xfe);//비동기 쓰기 실행 명령
	void syncWrite(u8 ID[], u8 IDN, u8 MemAddr, u8 *nDat, u8 nLen);//동기 쓰기 명령
	int writeByte(u8 ID, u8 MemAddr, u8 bDat);//1바이트 쓰기
	int writeWord(u8 ID, u8 MemAddr, u16 wDat);//2바이트 쓰기
	int Read(u8 ID, u8 MemAddr, u8 *nData, u8 nLen);//읽기 명령
	int readByte(u8 ID, u8 MemAddr);//1바이트 읽기
	int readWord(u8 ID, u8 MemAddr);//2바이트 읽기
	int Ping(u8 ID);//Ping 명령
	int syncReadPacketTx(u8 ID[], u8 IDN, u8 MemAddr, u8 nLen);//동기 읽기 명령 패킷 전송
	int syncReadPacketRx(u8 ID, u8 *nDat);//동기 읽기 응답 패킷 해석, 성공 시 메모리 바이트 수 반환, 실패 시 0 반환
	int syncReadRxPacketToByte();//1바이트 디코딩
	int syncReadRxPacketToWrod(u8 negBit=0);//2바이트 디코딩, negBit는 방향 비트, negBit=0이면 방향 없음
	void syncReadBegin(u8 IDN, u8 rxLen, u32 TimeOut);//동기 읽기 시작
	void syncReadEnd();//동기 읽기 종료
	int Reset(u8 ID);//서보 상태 초기화
	int Recal(u8 ID);//서보 중앙 위치 재보정

	u8 getState() { return u8Status; }
	u8 getLastError() { return u8Error; }

public:
	u8 Level;//서보 응답 레벨
	u8 End;//프로세서의 엔디안(Big/Little Endian)
	u8 u8Status;//서보 상태
	u8 u8Error;//통신 상태
	u8 syncReadRxPacketIndex;
	u8 syncReadRxPacketLen;
	u8 *syncReadRxPacket;
	u8 *syncReadRxBuff;
	u16 syncReadRxBuffLen;
	u16 syncReadRxBuffMax;
	u32 syncTimeOut;

protected:
	virtual int writeSCS(unsigned char *nDat, int nLen) = 0;
	virtual int readSCS(unsigned char *nDat, int nLen) = 0;
	virtual int readSCS(unsigned char *nDat, int nLen, unsigned long TimeOut) = 0;
	virtual int writeSCS(unsigned char bDat) = 0;
	virtual void rFlushSCS() = 0;
	virtual void wFlushSCS() = 0;

protected:
	void writeBuf(u8 ID, u8 MemAddr, u8 *nDat, u8 nLen, u8 Fun);
	void Host2SCS(u8 *DataL, u8* DataH, u16 Data);//16비트 데이터를 2개의 8비트 데이터로 분리
	u16	SCS2Host(u8 DataL, u8 DataH);//2개의 8비트 데이터를 1개의 16비트 데이터로 결합
	int	Ack(u8 ID);//응답 반환
	int checkHead();//프레임 헤더 검사
};
#endif
