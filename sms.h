#ifndef FDA51240_4423_4793_BF0A_29227CA73204
#define FDA51240_4423_4793_BF0A_29227CA73204

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <cassert>
#include <string>
#include <exception>
#include <stdexcept>



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
#define FAILED	-1
#define ERROR	-101
#define NOENOUGHMEMORY -401
#define QUIT -501


#define SMS_CHILDCODE_LEN	2 //子用户代码长度

#ifndef byte
	typedef unsigned char byte;
#endif 

#ifndef DWORD
	typedef unsigned long int DWORD;
#endif

class CSMSProtocol;
class CSMSStorage;
class CSMSDaemon;

typedef struct _SMSMessage{
	DWORD length;
	char SenderNumber[MOBILENUMBERLENGTH];
	char TargetNumber[MOBILENUMBERLENGTH];
	int FeeType;
	char FeeTargetNumber[MOBILENUMBERLENGTH];
	int SMSType;
	DWORD SMSBodyLength;
	char SMSBody[0];
}SMSMessage, *PSMSMessage;




}


#include "smsdaemon.h"
#include "smsstorage.h"
#include "smsprotocol.h"

#endif
