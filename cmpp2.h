#ifndef AKA_SMS_6D37B40E_AE4A_47B2_8757_91FF70855638
#define AKA_SMS_6D37B40E_AE4A_47B2_8757_91FF70855638

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t UINT32;
typedef uint64_t UINT64;
typedef	uint8_t	 UINT8;
typedef char	OCTETSTRING;

//COMMAND_ID定义
#define CMPP_CMD_CONNECT			0x00000001	//请求连接
#define CMPP_CMD_CONNECT_RESP		0x80000001	//请求连接应答
#define CMPP_CMD_TERMINATE			0x00000002	//终止连接
#define CMPP_CMD_TERMINATE_RESP		0x80000002	//终止连接应答
#define CMPP_CMD_SUBMIT				0x00000004	//提交短信
#define CMPP_CMD_SUBMIT_RESP		0x80000004	//提交短信应答
#define CMPP_CMD_DELIVER			0x00000005	//短信下发
#define CMPP_CMD_DELIVER_RESP		0x80000005	//下发短信应答
#define CMPP_CMD_QUERY				0x00000006	//发送短信状态查询
#define CMPP_CMD_QUERY_RESP			0x80000006	//发送短信状态查询应答
#define CMPP_CMD_CANCEL				0x00000007	//删除短信
#define CMPP_CMD_CANCEL_RESP		0x80000007	//删除短信应答
#define CMPP_CMD_ACTIVE_TEST		0x00000008	//激活测试
#define CMPP_CMD_ACTIVE_TEST_RESP	0x80000008	//激活测试应答

//端口号
#define CMPP_LONGCONNECTION_PORT	7890	//长连接（SP与网关间）
#define CMPP_SHORTCONNECTION_PORT	7900	//短连接（SP与网关间或网关之间）

// 消息头(所有消息公共包头)
typedef struct _CMPP_HEADER{
	UINT32 Total_Length;	//消息总长度(含消息头及消息体)
	UINT32 Command_Id;	//命令或响应类型
	UINT32 Sequence_Id;	//消息流水号,顺序累加,步长为1,循环使用（一对请求和应答消息的流水号必须相同）
}CMPP_HEADER, *PCMPP_HEADER;

//SP请求连接到ISMG（CMPP_CONNECT）操作 (SP-> ISMG)
typedef struct _CMPP_CONNECT{
	OCTETSTRING Source_Addr[6]; //源地址，此处为SP_Id，即SP的企业代码
	OCTETSTRING AuthenticatorSource[16]; /*	Octet String	用于鉴别源地址。其值通过单向MD5 hash计算得出，表示如下：
		AuthenticatorSource =
		MD5（Source_Addr+9 字节的0 +shared secret+timestamp）
		Shared secret 由中国移动与源地址实体事先商定，timestamp格式为：MMDDHHMMSS，即月日时分秒，10位。
		*/
	UINT8 Version;	//1	Unsigned Integer	双方协商的版本号(高位4bit表示主版本号,低位4bit表示次版本号)
	UINT32  Timestamp;	//4	Unsigned Integer	时间戳的明文,由客户端产生,格式为MMDDHHMMSS，即月日时分秒，10位(十进制）数字的整型，右对齐 。

}CMPP_CONNECT, *PCMPP_CONNECT;


//CMPP_CONNECT_RESP消息定义（ISMG -> SP）
typedef struct _CMPP_CONNECT_RESP{
	UINT8 Status; //	状态
/*
0：正确
1：消息结构错
 2：非法源地址
 3：认证错
 4：版本太高
  5~ ：其他错误
*/
	OCTETSTRING AuthenticatorISMG[16]; //	Octet String	ISMG认证码，用于鉴别ISMG。
/*
其值通过单向MD5 hash计算得出，表示如下：
AuthenticatorISMG =MD5（Status+AuthenticatorSource+shared secret），Shared secret 由中国移动与源地址实体事先商定，AuthenticatorSource为源地址实体发送给ISMG的对应消息CMPP_Connect中的值。
 认证出错时，此项为空。
*/
	UINT8 Version; //	1	Unsigned Integer	服务器支持的最高版本号

}CMPP_CONNECT_RESP, *PCMPP_CONNECT_RESP;

/*
CMPP_TERMINATE消息定义（SP -> ISMG或ISMG -> SP）
无消息体。
CMPP_TERMINATE_RESP消息定义（SP -> ISMG或ISMG -> SP）
无消息体。
*/

//SP向ISMG提交短信（CMPP_SUBMIT）操作 (SP->ISMG)
typedef struct _CMPP_SUBMIT_PART1 {
	UINT64 Msg_Id;		//	8	Unsigned Integer	信息标识，由SP侧短信网关本身产生，本处填空。
	UINT8 Pk_total;	//1	Unsigned Integer	相同Msg_Id的信息总条数，从1开始
	UINT8 Pk_number;	//1	Unsigned Integer	相同Msg_Id的信息序号，从1开始
	UINT8 Registered_Delivery;	//1	Unsigned Integer	是否要求返回状态确认报告：
/*
0：不需要
1：需要
2：产生SMC话单
 （该类型短信仅供网关计费使用，不发送给目的终端)
*/
	UINT8 Msg_level;	//1	Unsigned Integer	信息级别
	OCTETSTRING Service_Id[10]; //	Octet String	业务类型，是数字、字母和符号的组合。
	UINT8 Fee_UserType; //	1	Unsigned Integer	计费用户类型字段
/*
0：对目的终端MSISDN计费；
1：对源终端MSISDN计费；
2：对SP计费;
3：表示本字段无效，对谁计费参见Fee_terminal_Id字段。
*/
	OCTETSTRING Fee_terminal_Id[21];	//Unsigned Integer	被计费用户的号码（如本字节填空，则表示本字段无效，对谁计费参见Fee_UserType字段，本字段与Fee_UserType字段互斥）
	UINT8 TP_pId; //	1	Unsigned Integer	GSM协议类型。详细是解释请参考GSM03.40中的9.2.3.9
	UINT8 TP_udhi; //	1	Unsigned Integer	GSM协议类型。详细是解释请参考GSM03.40中的9.2.3.23,仅使用1位，右对齐
	UINT8 Msg_Fmt; //	1	Unsigned Integer	信息格式
/*
  0：ASCII串
  3：短信写卡操作
  4：二进制信息
  8：UCS2编码
15：含GB汉字  。。。。。。 
*/
	OCTETSTRING Msg_src[6];	//	Octet String	信息内容来源(SP_Id)
	OCTETSTRING FeeType[2];	//Octet String	资费类别
/*
01：对"计费用户号码"免费
02：对"计费用户号码"按条计信息费
03：对"计费用户号码"按包月收取信息费
04：对"计费用户号码"的信息费封顶
05：对"计费用户号码"的收费是由SP实现
*/
	OCTETSTRING FeeCode[6]; //	Octet String	资费代码（以分为单位）
	OCTETSTRING ValId_Time[17]; //	Octet String	存活有效期，格式遵循SMPP3.3协议
	OCTETSTRING At_Time[17]; //	Octet String	定时发送时间，格式遵循SMPP3.3协议
	OCTETSTRING Src_Id[21]; 	//Octet String	源号码
/*
SP的服务代码或前缀为服务代码的长号码, 网关将该号码完整的填到SMPP协议Submit_SM消息相应的source_addr字段，该号码最终在用户手机上显示为短消息的主叫号码
*/
}CMPP_SUBMIT_PART1, *PCMPP_SUBMIT_PART1;

typedef struct _CMPP_SUBMIT_PART2 {
	UINT8 DestUsr_tl;//	1	Unsigned Integer	接收信息的用户数量(小于100个用户)
}CMPP_SUBMIT_PART2, *PCMPP_SUBMIT_PART2;

typedef struct _CMPP_SUBMIT_PART3 {
	OCTETSTRING Dest_terminal_Id[0][21]; //	21*DestUsr_tl	Octet String	接收短信的MSISDN号码
}CMPP_SUBMIT_PART3, *PCMPP_SUBMIT_PART3;

typedef struct _CMPP_SUBMIT_PART4 {
	UINT8 Msg_Length;	//1	Unsigned Integer	信息长度(Msg_Fmt值为0时：<160个字节；其它<=140个字节)
}CMPP_SUBMIT_PART4, *PCMPP_SUBMIT_PART4;

typedef struct _CMPP_SUBMIT_PART5 {
	OCTETSTRING Msg_Content[0];//	Msg_length	Octet String	信息内容
}CMPP_SUBMIT_PART5, *PCMPP_SUBMIT_PART5;

typedef struct _CMPP_SUBMIT_PART6 {
	OCTETSTRING Reserve[8]; //	8	Octet String	保留
}CMPP_SUBMIT_PART6, *PCMPP_SUBMIT_PART6;

//CMPP_SUBMIT_RESP消息定义（ISMG -> SP）
typedef struct _CMPP_SUBMIT_RESP {
	UINT64 Msg_Id;//	8	Unsigned Integer	信息标识，生成算法如下：
/*
采用64位（8字节）的整数：
（1）	时间（格式为MMDDHHMMSS，即月日时分秒）：bit64~bit39，其中
bit64~bit61：月份的二进制表示；
bit60~bit56：日的二进制表示；
bit55~bit51：小时的二进制表示；
bit50~bit45：分的二进制表示；
bit44~bit39：秒的二进制表示；
（2）	短信网关代码：bit38~bit17，把短信网关的代码转换为整数填写到该字段中。
（3）	序列号：bit16~bit1，顺序增加，步长为1，循环使用。
各部分如不能填满，左补零，右对齐。
（SP根据请求和应答消息的Sequence_Id一致性就可得到CMPP_Submit消息的Msg_Id）
*/
	UINT8 Result;  //	1	Unsigned Integer	结果
/*
0：正确
1：消息结构错
 2：命令字错
 3：消息序号重复
4：消息长度错
5：资费代码错
6：超过最大信息长
7：业务代码错
8：流量控制错
9~ ：其他错误
*/
}CMPP_SUBMIT_RESP , *PCMPP_SUBMIT_RESP;

//SP向ISMG查询发送短信状态（CMPP_QUERY）操作
typedef struct _CMPP_QUERY{
	OCTETSTRING Time[8];//	8	Octet String	时间YYYYMMDD(精确至日)
	UINT8 Query_Type;//	1	Unsigned Integer	查询类别
/*
0：总数查询
1：按业务类型查询 
*/
	OCTETSTRING Query_Code[10]; //	10	Octet String	查询码
/*
当Query_Type为0时，此项无效；当Query_Type为1时，此项填写业务类型Service_Id. 
*/
	OCTETSTRING Reserve[8]; //	8	Octet String	保留
}CMPP_QUERY, *PCMPP_QUERY;

//CMPP_QUERY_RESP消息的定义（ISMG -> SP）
typedef struct _CMPP_QUERY_RESP{
	UINT8 Query_Type; //	1	Unsigned Integer	查询类别
/*
0：总数查询
1：按业务类型查询 
*/
	OCTETSTRING Query_Code[10]; //	10	Octet String	查询码
	UINT32 MT_TLMsg; //	4	Unsigned Integer	从SP接收信息总数
	UINT32 MT_Tlusr; //	4	Unsigned Integer	从SP接收用户总数
	UINT32 MT_Scs; //	4	Unsigned Integer	成功转发数量
	UINT32 MT_WT; //	4	Unsigned Integer	待转发数量
	UINT32 MT_FL; //	4	Unsigned Integer	转发失败数量
	UINT32 MO_Scs; //	4	Unsigned Integer	向SP成功送达数量
	UINT32 MO_WT; //	4	Unsigned Integer	向SP待送达数量
	UINT32 MO_FL; //	4	Unsigned Integer	向SP送达失败数量
}CMPP_QUERY_RESP, *PCMPP_QUERY_RESP;

//ISMG向SP送交短信（CMPP_DELIVER）操作
typedef struct _CMPP_DELIVER_PART1{
	UINT64 Msg_Id; //	8	Unsigned Integer	信息标识
/*
生成算法如下：
采用64位（8字节）的整数：
（1）	时间（格式为MMDDHHMMSS，即月日时分秒）：bit64~bit39，其中
bit64~bit61：月份的二进制表示；
bit60~bit56：日的二进制表示；
bit55~bit51：小时的二进制表示；
bit50~bit45：分的二进制表示；
bit44~bit39：秒的二进制表示；
（2）	短信网关代码：bit38~bit17，把短信网关的代码转换为整数填写到该字段中。
（3）	序列号：bit16~bit1，顺序增加，步长为1，循环使用。
各部分如不能填满，左补零，右对齐。
*/
	OCTETSTRING Dest_Id[21]; //	21	Octet String	目的号码 

/*
SP的服务代码，一般4--6位，或者是前缀为服务代码的长号码；该号码是手机用户短消息的被叫号码。
*/
	OCTETSTRING Service_Id[10]; //	10	Octet String	业务类型，是数字、字母和符号的组合。
	UINT8 TP_pid; //	1	Unsigned Integer	GSM协议类型。详细解释请参考GSM03.40中的9.2.3.9
	UINT8 TP_udhi; //	1	Unsigned Integer	GSM协议类型。详细解释请参考GSM03.40中的9.2.3.23，仅使用1位，右对齐
	UINT8 Msg_Fmt; //	1	Unsigned Integer	信息格式
/*
  0：ASCII串
  3：短信写卡操作
  4：二进制信息
  8：UCS2编码
15：含GB汉字   
*/
	OCTETSTRING Src_terminal_Id[21]; //	21	Octet String	源终端MSISDN号码（状态报告时填为CMPP_SUBMIT消息的目的终端号码）
	UINT8 Registered_Delivery; //	1	Unsigned Integer	是否为状态报告
/*
0：非状态报告
1：状态报告
*/
	UINT8 Msg_Length; //	1	Unsigned Integer	消息长度
	OCTETSTRING Msg_Content[0]; //	Msg_length	Octet String	消息内容
}CMPP_DELIVER_PART1, *PCMPP_DELIVER_PART1;

typedef struct CMPP_DELIVER_PART2 {
	OCTETSTRING Reserved[8]; //	8	Octet String	保留项
}CMPP_DELIVER_PART2, *PCMPP_DELIVER_PART2;

//状态报告内容定义：
typedef struct _CMPP_STATSREPORT{
	UINT64 Msg_Id; //	8	Unsigned Integer	信息标识
/*
	SP提交短信（CMPP_SUBMIT）操作时，与SP相连的ISMG产生的Msg_Id。
*/
	OCTETSTRING Stat[7]; //	7	Octet String	发送短信的应答结果，含义与SMPP协议要求中stat字段定义相同，详见表一。SP根据该字段确定CMPP_SUBMIT消息的处理状态。
	OCTETSTRING Submit_time[10]; //	10	Octet String	YYMMDDHHMM（YY为年的后两位00-99，MM：01-12，DD：01-31，HH：00-23，MM：00-59）
	OCTETSTRING Done_time[10]; //	10	Octet String	YYMMDDHHMM
	OCTETSTRING Dest_terminal_Id[21]; //	21	Octet String	目的终端MSISDN号码(SP发送CMPP_SUBMIT消息的目标终端)
	UINT32 SMSC_sequence;//	4	Unsigned Integer	取自SMSC发送状态报告的消息体中的消息标识。
}CMPP_STATSREPORT, *PCMPP_STATSREPORT;

//CMPP_DELIVER_RESP消息定义（SP -> ISMG）
typedef struct _CMPP_DELIVER_RESP {
	UINT64 Msg_Id; //	8	Unsigned Integer	信息标识
/*
（CMPP_DELIVER中的Msg_Id字段）
*/
	UINT8 Result; //	1	Unsigned Integer	结果
/*
0：正确
1：消息结构错
 2：命令字错
 3：消息序号重复
4：消息长度错
5：资费代码错
6：超过最大信息长
7：业务代码错
8: 流量控制错
9~ ：其他错误
*/
} CMPP_DELIVER_RESP, *PCMPP_DELIVER_RESP;

//SP向ISMG发起删除短信（CMPP_CANCEL）操作
typedef struct _CMPP_CANCEL {
	UINT64 Msg_Id; //	8	Unsigned Integer	信息标识（SP想要删除的信息标识）
}CMPP_CANCEL , *PCMPP_CANCEL;

//CMPP_CANCEL_RESP消息定义（ISMG ' SP）
typedef struct _CMPP_CANCEL_RESP{
	UINT8 Success_Id; //	1	Unsigned Integer	成功标识
/*
0：成功
1：失败
*/
}CMPP_CANCEL_RESP, *PCMPP_CANCEL_RESP;

// CMPP_ACTIVE_TEST_RESP定义（SP -> ISMG或ISMG -> SP）
typedef struct _CMPP_ACTIVE_TEST_RESP{
	UINT8 Reserved;
}CMPP_ACTIVE_TEST_RESP, *PCMPP_ACTIVE_TEST_RESP;

#ifdef __cplusplus
}
#endif
#endif
