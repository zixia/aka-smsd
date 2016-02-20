#ifndef LIB_BBS_SMS
#define LIB_BBS_SMS

#define SMS_USER_LEN	19	//网关登录号最大长度
#define SMS_PASS_LEN	49	//网关登陆密码最大长度
#define SMS_VALID_LEN	6	//认证码长度
#define MOBILENUMBERLENGTH 16
#define SMS_BBS_ID_LEN	13	//用户ID最大长度

#define SMS_CMD_GWSMS 1
#define SMS_CMD_REGISTER 2
#define SMS_CMD_UNREGISTER 3

#define SMS_CMD_OK 101		//操作成功
#define SMS_CMD_ERR 102		//操作失败
#define SMS_CMD_HEAD_LENGTH_ERR 103	//包头错误
#define SMS_CMD_DB_ERROR	104		//数据库操作错误
#define SMS_CMD_SMS_VALIDATE_FAILED	105		//短信无法通过发送检查
#define SMS_CMD_LENGTH_ERR	106			//消息包长度错误
#define SMS_CMD_NO_VALIDCODE	107		//无认证码供验证

#define SMS_MSG_TYPE_TXT		0	//普通文本短信

#define SMS_ERR_MSGTYPE			-10	//短信类型错误

#define SMS_ERR_GWCLOSED		-100	//网关断开连接
#define SMS_ERR_MEMORY			-200	//内存分配失败

#define SMS_ERR_DNS		-301 //DNS解析错误
#define SMS_ERR_TCPPROTOCOL	-302 //找不到tcp协议
#define SMS_ERR_SOCKET		-303 //无法建立socket
#define SMS_ERR_CONNECT		-304 //连接失败

#define SMS_ERR_LOGIN		-305 //登录网关失败

#define SMS_ERR_NOT_CONNECTED	-501 //连接尚未建立

#ifndef DWORD
typedef unsigned long int DWORD;
#endif

#ifndef byte
typedef unsigned char byte;
#endif

typedef struct _SMS_BINDREQUEST { //取网关的手机绑定请求信息
    char userID[SMS_BBS_ID_LEN+1];
    char MobileNo[MOBILENUMBERLENGTH+1];
}SMS_BINDREQUEST, *PSMS_BINDREQUEST;
	
typedef struct _SMS_GWSMS { //取网关传递的手机发往bbs的短信
    DWORD userBBSCode;
    char SrcMobileNo[MOBILENUMBERLENGTH+1];
	byte MsgType;
    DWORD MsgTxtLen;
    char MsgTxt[0];
}SMS_GWSMS, *PSMS_GWSMS;

//初始化内部结构和数据
int sms_init();

//建立socket连接后登录网关
int sms_login(DWORD serialNo, char* gatewayAddress, unsigned int gatewayPort, char* user, char* pass);  

//请求网关向手机发送绑定注册码
int sms_sendRegisterNumber(DWORD  serialNo, int pid, char* mobilePhoneNumber, char* userID); 

//请求网关检查输入的绑定注册码是否正确
int sms_checkRegisterNumber(DWORD serialNo, int pid, char* mobilePhoneNumber, char* userID, char* validationCode);

//请求网关取消手机绑定
int sms_unRegister(DWORD serialNo, int pid, char* mobilePhoneNumber, char* userID);

//请求网关发送手机短信
int sms_sendSMS(DWORD serialNo, int pid,DWORD userBBSCode,  char *userID, char * sourcePhoneNumber, char* targetPhoneNumber, char* content, int contentLen, int contentType);

//回应网关的手机绑定请求
int sms_replyGWRegister(DWORD serialNo, int pid ,int isSucceed);

//从网关读取操作回复、短信等信息
int sms_getReply(DWORD* pSerialNo, int *pPid, int *pReplyType, void** pBuf);

//关闭系统
int sms_exit();

//获取连接句柄
int sms_getsocket();

#endif
