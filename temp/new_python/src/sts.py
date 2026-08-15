#!/usr/bin/env python

from .stservo_def import *
from .protocol_packet_handler import *
from .group_sync_read import *
from .group_sync_write import *

#bps_def
STS_1M = 0
STS_0_5M = 1
STS_250K = 2
STS_128K = 3
STS_115200 = 4
STS_76800 = 5
STS_57600 = 6
STS_38400 = 7

#memory table definition
#-------EPROM(읽기 전용)--------
STS_MODEL_L = 3
STS_MODEL_H = 4

#-------EPROM(읽기와 쓰기)--------
STS_ID = 5
STS_BAUD_RATE = 6
STS_MIN_ANGLE_LIMIT_L = 9
STS_MIN_ANGLE_LIMIT_H = 10
STS_MAX_ANGLE_LIMIT_L = 11
STS_MAX_ANGLE_LIMIT_H = 12
STS_CW_DEAD = 26
STS_CCW_DEAD = 27
STS_OFS_L = 31
STS_OFS_H = 32
STS_MODE = 33

#-------SRAM(읽기와 쓰기)--------
STS_TORQUE_ENABLE = 40
STS_ACC = 41
STS_GOAL_POSITION_L = 42
STS_GOAL_POSITION_H = 43
STS_GOAL_TIME_L = 44
STS_GOAL_TIME_H = 45
STS_GOAL_SPEED_L = 46
STS_GOAL_SPEED_H = 47
STS_LOCK = 55

#-------SRAM(읽기 전용)--------
STS_PRESENT_POSITION_L = 56
STS_PRESENT_POSITION_H = 57
STS_PRESENT_SPEED_L = 58
STS_PRESENT_SPEED_H = 59
STS_PRESENT_LOAD_L = 60
STS_PRESENT_LOAD_H = 61
STS_PRESENT_VOLTAGE = 62
STS_PRESENT_TEMPERATURE = 63
STS_MOVING = 66
STS_PRESENT_CURRENT_L = 69
STS_PRESENT_CURRENT_H = 70
#------------------------------------------------------------------------------------

BROADCAST_ID = 0xFE  # 254
MAX_ID = 0xFC  # 252
STS_END = 0

# Instruction for STS Protocol
INST_PING = 1
INST_READ = 2
INST_WRITE = 3
INST_REG_WRITE = 4
INST_ACTION = 5
INST_SYNC_WRITE = 131  # 0x83
INST_SYNC_READ = 130  # 0x82

# Communication Result
COMM_SUCCESS = 0  # tx or rx packet communication success
COMM_PORT_BUSY = -1  # Port is busy (in use)
COMM_TX_FAIL = -2  # Failed transmit instruction packet
COMM_RX_FAIL = -3  # Failed get status packet
COMM_TX_ERROR = -4  # Incorrect instruction packet
COMM_RX_WAITING = -5  # Now recieving status packet
COMM_RX_TIMEOUT = -6  # There is no status packet
COMM_RX_CORRUPT = -7  # Incorrect status packet
COMM_NOT_AVAILABLE = -9  #
#------------------------------------------------------------------------------------

class sts(protocol_packet_handler):
    def __init__(self, portHandler):
        protocol_packet_handler.__init__(self, portHandler, 0)
        self.groupSyncWrite = GroupSyncWrite(self, STS_ACC, 7)

    def WritePosEx(self, sts_id, position, speed, acc):
        txpacket = [acc, self.sts_lobyte(position), self.sts_hibyte(position), 0, 0, self.sts_lobyte(speed), self.sts_hibyte(speed)]
        return self.writeTxRx(sts_id, STS_ACC, len(txpacket), txpacket)

    def ReadPos(self, sts_id):
        sts_present_position, sts_comm_result, sts_error = self.read2ByteTxRx(sts_id, STS_PRESENT_POSITION_L)
        return self.sts_tohost(sts_present_position, 15), sts_comm_result, sts_error

    def ReadSpeed(self, sts_id):
        sts_present_speed, sts_comm_result, sts_error = self.read2ByteTxRx(sts_id, STS_PRESENT_SPEED_L)
        return self.sts_tohost(sts_present_speed, 15), sts_comm_result, sts_error

    def ReadPosSpeed(self, sts_id):
        sts_present_position_speed, sts_comm_result, sts_error = self.read4ByteTxRx(sts_id, STS_PRESENT_POSITION_L)
        sts_present_position = self.sts_loword(sts_present_position_speed)
        sts_present_speed = self.sts_hiword(sts_present_position_speed)
        return self.sts_tohost(sts_present_position, 15), self.sts_tohost(sts_present_speed, 15), sts_comm_result, sts_error

    def ReadMoving(self, sts_id):
        moving, sts_comm_result, sts_error = self.read1ByteTxRx(sts_id, STS_MOVING)
        return moving, sts_comm_result, sts_error

    def SyncWritePosEx(self, sts_id, position, speed, acc):
        txpacket = [acc, self.sts_lobyte(position), self.sts_hibyte(position), 0, 0, self.sts_lobyte(speed), self.sts_hibyte(speed)]
        return self.groupSyncWrite.addParam(sts_id, txpacket)

    def RegWritePosEx(self, sts_id, position, speed, acc):
        txpacket = [acc, self.sts_lobyte(position), self.sts_hibyte(position), 0, 0, self.sts_lobyte(speed), self.sts_hibyte(speed)]
        return self.regWriteTxRx(sts_id, STS_ACC, len(txpacket), txpacket)

    def RegAction(self):
        return self.action(BROADCAST_ID)

    def WheelMode(self, sts_id):
        return self.write1ByteTxRx(sts_id, STS_MODE, 1)

    def WriteSpec(self, sts_id, speed, acc):
        speed = self.sts_toscs(speed, 15)
        txpacket = [acc, 0, 0, 0, 0, self.sts_lobyte(speed), self.sts_hibyte(speed)]
        return self.writeTxRx(sts_id, STS_ACC, len(txpacket), txpacket)

    def LockEprom(self, sts_id):
        return self.write1ByteTxRx(sts_id, STS_LOCK, 1)

    def unLockEprom(self, sts_id):
        return self.write1ByteTxRx(sts_id, STS_LOCK, 0)

