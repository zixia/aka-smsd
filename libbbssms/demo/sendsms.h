#ifndef SENDSMS_H
#define SENDSMS_H
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include "libbbssms.h"

#define SMS_SHM_SIZE (1024*50)

struct BBS_header{
  char Type;
  DWORD SerialNo;
  DWORD pid;
  DWORD BodyLength;   //总Packet长度
};

#define BBS_CMD_REG 1
#define BBS_CMD_CHECK 2
#define BBS_CMD_UNREG 3
#define BBS_CMD_BBSSEND 4

struct BBS_RegMobileNoPacket {
    char phone[MOBILENUMBERLENGTH+1];
    char id[SMS_BBS_ID_LEN+1];
};
struct BBS_CheckMobileNoPacket {
    char phone[MOBILENUMBERLENGTH+1];
    char id[SMS_BBS_ID_LEN+1];
    char code[MOBILENUMBERLENGTH+1];
};
struct BBS_UnRegPacket { 
    char phone[MOBILENUMBERLENGTH+1];
    char id[SMS_BBS_ID_LEN+1];
};
struct BBS_BBSSendSMS { 
    DWORD uid;
    char phone[MOBILENUMBERLENGTH+1];
    char id[SMS_BBS_ID_LEN+1];
    char targetphone[MOBILENUMBERLENGTH+1];
    char content[200];
};

struct BBS_sms_shm_head {
    int sem;
    int total;
    int length;
} ;

#endif

