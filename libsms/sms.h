
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


typedef struct _SMSMessage{
	unsigned int length;
	char SenderNumber[MOBILENUMBERLENGTH];
	char TargetNumber[MOBILENUMBERLENGTH];
	int FeeType;
	char FeeTargetNumber[MOBILENUMBERLENGTH];
	int SMSType;
	unsigned int SMSBodyLength;
	char SMSBody[0];
}SMSMessage, *PSMSMessage;

#ifndef byte
typedef  unsigned char byte;
#endif

#define FEETYPE_1 1

//短信类型常量定义


//消息类型常量定义
#define MSGTYPE_SM		0
#define MSGTYPE_SMR		1
#define MSGTYPE_SMS		2
#define MSGTYPE_RM		3
#define MSGTYPE_CD		4
#define MSGTYPE_CDR		5
#define MSGTYPE_PWD		10    //验证连接密码

#define CONNECTION_USER_LEN		20
#define CONNECTION_PASSWORD_LEN 50



typedef struct _SMSChildProtocolHead{
	byte msgTypeID;	//消息类型
	byte SMSSerialNo[4];		//由消息主动产生方生成的信息序列号，高位在前
	byte msgLength[4]; //不包括头信息的消息体长度（字节数）高位在前，低位在后
	byte unused[4];	//保留，将来做checksum
} SMSChildProtocolHead, *PSMSChildProtocolHead;

typedef struct _SMSChildProtocolCommon { //用于处理消息头的结构
	SMSChildProtocolHead head;		//头信息	
} SMSChildProtocolCommon, *PSMSChildProtocolCommon;


typedef struct _SMSChildProtocolSendMessage { //下游网关的发送短信消息
	SMSChildProtocolHead head;		//头信息
	char senderNo[MOBILENUMBERLENGTH+1];  //短信的发送方号码
	char targetNo[MOBILENUMBERLENGTH+1];	//短信的接收方号码
	char feeTargetNo[MOBILENUMBERLENGTH+1];		//本次短信计费的对象号码
	byte feeTypeID;				//		收费类型代码
	byte smsTypeID;				//		短信类型
	byte smsBodyLength[4];		//短信内容长度（字节） 高位在前，低位在后
	char smsBody[0];		//短信内容
}SMSChildProtocolSendMessage, *PSMSChildProtocolSendMessage; 

#define MSG_OK					0
#define MSGERR_SENDERNO			11
#define MSGERR_TARGETNO			12
#define MSGERR_FEETARGETNO		21
#define MSGERR_FEETYPE			22
#define MSGERR_SMSTYPE			31
#define MSGERR_SMSLEN			41
#define MSGERR_MSGTYPE			51

typedef struct _SMSChildProtocolSendMessgeReceived{ //本地网关接受到发送短信消息之后的返回
	SMSChildProtocolHead head;		//头信息
	byte SerialNo[4];			//由本地网关生成的信息序列号，高位在前
	byte ErrorNo;			 //对发送短信消息内容validate之后的结果
} SMSChildProtocolSendMessgeReceived, *PSMSChildProtocolSendMessgeReceived;

#define SEND_OK					0

typedef struct _SMSChildProtocolSendMessageSended{ //本地网关成功发送短信消息之后的返回
	SMSChildProtocolHead head;		//头信息
	byte SerialNo[4];			//由本地网关生成的信息序列号，高位在前
	byte ErrorNo;			 //发送结果
}SMSChildProtocolSendMessageSended, *PSMSChildProtocolSendMessageSended;

typedef struct _SMSChildProtocolReceivedMessage{ //本地网关转发给下游的短信
	SMSChildProtocolHead head;		//头信息
	char senderNo[MOBILENUMBERLENGTH+1];  //短信的发送方号码
	char targetNo[MOBILENUMBERLENGTH+1];	//短信的接收方号码
	byte smsTypeID;				//		短信类型
	byte smsLength[4];		//短信内容长度（字节） 高位在前，低位在后
	char smsBody[0];		//短信内容
}SMSChildProtocolReceivedMessage, *PSMSChildProtocolReceivedMessage;

typedef struct _SMSChildProtocolConnectionDetect{ //本地网关对下游进行状态检测
	SMSChildProtocolHead head;		//头信息
	byte SerialNo[4];			//由本地网关生成的信息序列号，高位在前
}	SMSChildProtocolConnectionDetect, *PSMSChildProtocolConnectionDetect;

typedef struct _SMSChildProtocolReply{ //下游对本地网关状态检测的回复
	SMSChildProtocolHead head;		//头信息
	byte SerialNo[4];			//由本地网关生成的信息序列号，高位在前
}	SMSChildProtocolReply, *PSMSChildProtocolReply;

typedef struct _SMSChildProtocolPassword{ //验证连接用户名和密码
	SMSChildProtocolHead head;		//头信息
	char user[CONNECTION_USER_LEN];			
	char password[CONNECTION_PASSWORD_LEN];
}	SMSChildProtocolPassword, *PSMSChildProtocolPassword;


inline unsigned long int sms_byteToLong(byte arg[4]) {
	long tmp;
	tmp=(arg[0]<<24)+(arg[1]<<16)+(arg[2]<<8)+arg[3];
	return tmp;
}

inline void sms_longToByte(byte* arg, unsigned long int num) {
	(arg)[0]=num>>24;
	(arg)[1]=(num<<8)>>24;
	(arg)[2]=(num<<16)>>24;
	(arg)[3]=(num<<24)>>24;
}

#define USER "test"
#define PASSWORD "test123"