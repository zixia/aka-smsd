#ifndef FDA51240_4423_4793_BF0A_29227CA73204
#define FDA51240_4423_4793_BF0A_29227CA73204

#include <stdint.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <cassert>
#include <string>
#include <exception>
#include <stdexcept>
#include <time.h>


namespace SMS{


#define MOBILENUMBERLENGTH 16  //号码长度
#define FEETYPE_DEFAULT 1;

#define SMSTYPE_TEXT	1;
#define SMSTYPE_BIN	 0

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

//返回值定义
#define SUCCESS 0
#define NOSEVEREERROR	1
#define TIMEOUT	-100
#define FAILED	-1
#define DONTDELETE FAILED
#define ERROR	-101
#define CONNECTIONLOST -102
#define RCVERR  -103
#define PARSE_ERROR	-102
#define NOENOUGHMEMORY -401
#define QUIT -501

#define DB_NAME "AKA"
#define DB_HOST "localhost"
#define DB_USER "aka"
#define DB_PASSWORD "aA3$;G(~cjKK"

#define SMSHOME "/opt/sms/"
#define SMSETCDIR SMSHOME "etc/"
		
#define SMS_PARENTID_LEN 80
#define SMS_FEECODE_LEN		10

#define SMS_MAXCHILDCODE_LEN 4	//最大子客户代码长度
#define SMS_MAXCHILDNAME_LEN 50 //最大子客户名称长度
	
typedef uint8_t byte;

typedef uint32_t DWORD;

class CSMSProtocol;
class CSMSStorage;
class CSMSDaemon;

typedef struct _SMSMessage{
	DWORD length;
	char SenderNumber[MOBILENUMBERLENGTH+1];
	char TargetNumber[MOBILENUMBERLENGTH+1];
	int FeeType;
	char FeeTargetNumber[MOBILENUMBERLENGTH+1];
	char childCode[SMS_MAXCHILDCODE_LEN+1];
	char parentID[SMS_PARENTID_LEN+1];
	int SMSType;
	time_t sendTime;
	time_t arriveTime;
	char serviceCode[SMS_FEECODE_LEN+1];
	DWORD SMSBodyLength;
	char SMSBody[0];
}SMSMessage, *PSMSMessage;




}


#include "smsdaemon.h"
#include "smsstorage.h"
#include "smsprotocol.h"

#endif
