
	typedef unsigned long int DWORD ;

	#define OAKSID_REQ                                                   0x00000000     // 请求消息类型标志
	#define OAKSID_ACK                                                   0x80000000     // 应答消息类型标志
	#define BUFLEN_MOBILENO                                               (15+1)       //手机号码长度 

	//其中,
	//发送包头定义为
	typedef  struct _tagOAKSREQHeader{
		 DWORD   dwType;                     //消息类型
		 DWORD   dwLength;                   //消息体长度(不包括头信息)
		 DWORD   dwCheckSum;                 //目前始终填0即可
	}OAKSREQHEADER,*POAKSREQHEADER; 
	//收费代码(FeeID)定义为
	//1                           免费(sp承担通道费)
	//2                           0.20元
	//4                           1.00元
	//25                          2.00元
	//0.5元的代码现在待定

	//发送包结构定义
	#define OAKSID_SM_ZIXIASENDTEXT                                      0x00000200     
	typedef struct{
		 OAKSREQHEADER header;                      
		 int  nSerialID;                                  //序列号id,直接在回复的时候写上.
		 char szSrcMobileNo[BUFLEN_MOBILENO];             //发送方手机号码
		 char szDstMobileNo[BUFLEN_MOBILENO];             //接收方手机号码
		 int  nFeeID;                                     //收费id, 定义见后面
		 int  nSendDate;                                  //发送日期，=0表示马上发送，其它定期发送，格式YYMMDDHHMI(其中YY、MM、DD均可为0，为0时表示每年、月、日)
		 int  lenText;                                    //消息数据长度，紧跟在消息体后面
	}OAKSREQSMZIXIASENDTEXT,*POAKSREQSMZIXIASENDTEXT; 


	//接收包头定义为
	typedef  struct _tagOAKSHeader{
		 DWORD   dwType;                     //消息类型
		 DWORD   dwLength;                   //消息体长度(不包括头信息)
		 DWORD   dwResult;                   //结果返回
	}OAKSACKHEADER,*POAKSACKHEADER; 

	//回复包结构定义
	typedef struct{
		 OAKSACKHEADER header;
		 int  nSerialID;                                //序列号id, 回复的时候添写.
	}OAKSACKSMZIXIASENDTEXT,*POAKSACKSMZIXIASENDTEXT;



	//返回值放在header中的dwResult ， 定义为：
	#define		OAKSBIT_SUCCESS								1				//发送成功
	
	#define     OAKSERR_SM_SOCKET                           410            //服务器Socket错误
	#define     OAKSERR_SM_INVALIDID                        411            //用户手机号码无效
	#define     OAKSERR_SM_INVALIDSENDNO                    412            //发送方手机号码无效
	#define     OAKSERR_SM_INVALIDRECVNO                    413            //接收方手机号码无效
	#define     OAKSERR_SM_MSGTOOLENGTH                     417             //文字信息过长或为空（无效）
	#define     OAKSERR_SM_MOBILESVRERR                     420             //与中国移动网关通讯失败
	#define     OAKSERR_SM_BUSY                             503            //服务器忙
	#define     OAKSERR_SM_INVALIDMOBILENO                  508            //手机号码错误
	#define     OAKSERR_SM_PASTDATE                         510            //定期发送时间已经过了
	#define     OAKSERR_SM_INVALIDSEND                      525            //移动和联通的手机号码不能互相发送
	#define     OAKSERR_SM_INVALIDMSGID                     526            //服务代码错误
	#define     OAKSERR_SM_DISABLEMSGID                     532            //这项服务暂停
	 
const char host_18dx[]="61.48.28.121";
const char port_18dx[]="6002";

