#ifndef SMS_BBS_6DA06848BE2D41f29E8FA85F83CB5AD7
#define SMS_BBS_6DA06848BE2D41f29E8FA85F83CB5AD7

namespace SMS{

typedef struct _SMS_BBS_HEADER{	//
	  byte Type;
	  byte SerialNo[4];
	  byte pid[4];
	  byte msgLength[4];   //总Packet长度
} SMS_BBS_HEADER, *PSMS_BBS_HEADER;


//返回值
#define SMS_BBS_CMD_LOGIN 1
#define SMS_BBS_CMD_OK 101
#define SMS_BBS_CMD_ERR 102
#define SMS_BBS_CMD_HEAD_LENGTH_ERR 103	//包头错误
#define SMS_BBS_CMD_DB_ERROR	104		//数据库操作错误
#define SMS_BBS_CMD_SMS_VALIDATE_FAILED	105		//短信无法通过发送检查
#define SMS_BBS_CMD_LENGTH_ERR	106			//消息包长度错误
#define SMS_BBS_CMD_NO_VALIDCODE	107		//无认证码供验证
#define SMS_BBS_CMD_NO_SUCHMOBILE	108	//取消认证码时发现此手机未认证


#define SMS_BBS_CMD_LOGOUT 2
#define SMS_BBS_CMD_REG 3
#define SMS_BBS_CMD_CHECK 4
#define SMS_BBS_CMD_UNREG 5
#define SMS_BBS_CMD_REQUEST 6
#define SMS_BBS_CMD_REQUESTREPLY 7
#define SMS_BBS_CMD_BBSSEND 8
#define SMS_BBS_CMD_GWSEND 9

#define SMS_BBS_USER_LEN	19	
#define SMS_BBS_PASS_LEN	49
#define SMS_BBS_VALID_LEN	10	//认证码长度
#define SMS_BBS_TYPE_LEN	1	//短信类型长度


//短消息类型代码
#define SMS_BBS_TYPE_COMMON		0	//普通短信
#define SMS_BBS_TYPE_REGISTER	1	//认证短信
#define SMS_BBS_TYPE_NONE		-1
//消息类型


typedef struct _SMS_BBS_LOGINPACKET {	//BBS连接请求
		SMS_BBS_HEADER header;
	    char user[SMS_BBS_USER_LEN+1];
	    char password[SMS_BBS_PASS_LEN+1];
} SMS_BBS_LOGINPACKET, *PSMS_BBS_LOGINPACKET;

typedef struct _SMS_BBS_REGISTERMOBILEPACKET { //BBS请求网关发送手机绑定短信
		SMS_BBS_HEADER header;
	    char MobileNo[MOBILENUMBERLENGTH+1];
} SMS_BBS_REGISTERMOBILEPACKET,*PSMS_BBS_REGISTERMOBILEPACKET;

typedef struct _SMS_BBS_REGISTERVALIDATIONPACKET { //BBS请求网关检查手机绑定码
		SMS_BBS_HEADER header;
	    char MobileNo[MOBILENUMBERLENGTH+1];
        char ValidateNo[SMS_BBS_VALID_LEN+1];
} SMS_BBS_REGISTERVALIDATIONPACKET, *PSMS_BBS_REGISTERVALIDATIONPACKET;

typedef struct _SMS_BBS_UNREGISTERMOBILEPACKET { //BBS请求网关取消手机绑定
		SMS_BBS_HEADER header;
	    char MobileNo[MOBILENUMBERLENGTH+1];
}SMS_BBS_UNREGISTERMOBILEPACKET, *PSMS_BBS_UNREGISTERMOBILEPACKET;

typedef struct _SMS_BBS_BINDREQUESTPACKET { //网关要求BBS绑定手机号码
		SMS_BBS_HEADER header;
	    byte UserID[4];
        char MobileNo[MOBILENUMBERLENGTH+1];
	    byte Bind;
}SMS_BBS_BINDREQUESTPACKET, *PSMS_BBS_BINDREQUESTPACKET;

typedef struct _SMS_BBS_BINDREQUESTREPLYPACKET { //Type=7
		SMS_BBS_HEADER header;
	    char MobileNo[MOBILENUMBERLENGTH+1];
        byte isSucceed;
}SMS_BBS_BINDREQUESTREPLYPACKET, *PSMS_BBS_BINDREQUESTREPLYPACKET;

typedef struct _SMS_BBS_BBSSENDSMS { //Type=8
		SMS_BBS_HEADER header;
	    byte UserID[4];
	    char SrcMobileNo[MOBILENUMBERLENGTH+1];
		char DstMobileNo[MOBILENUMBERLENGTH+1];
		byte MsgTxtLen[4];
		char MsgTxt[0];
}SMS_BBS_BBSSENDSMS,*PSMS_BBS_BBSSENDSMS;

typedef struct _SMS_BBS_GWSENDSMS { //Type=9
		SMS_BBS_HEADER header;
	    byte UserID[4];
        char SrcMobileNo[MOBILENUMBERLENGTH+1];
	    byte MsgTxtLen[4];
		char MsgTxt[0];
}SMS_BBS_GWSENDSMS, *PSMS_BBS_GWSENDSMS;


inline unsigned long int sms_byteToLong(byte arg[4]) {
	long tmp;
	tmp=(arg[0]<<24)+(arg[1]<<16)+(arg[2]<<8)+arg[3];
	return tmp;
}

inline void sms_longToByte(byte* arg, DWORD num) {
	arg[0]=num>>24;
	arg[1]=(num>>16) & 0xff;
	arg[2]=(num>>8) & 0xff;
	arg[3]=num & 0xff;
}

}

#endif
