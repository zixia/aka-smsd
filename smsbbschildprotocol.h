/*
 * 
 *
 *
 */
#ifndef SMS_51AD2DAD_B486_4391_A641_84C4719FF7DE
#define SMS_51AD2DAD_B486_4391_A641_84C4719FF7DE

#include <cc++/socket.h>
#include <sqlplus.hh>

#include "sms.h"
#include "smsbbschildprivilegechecker.h"
#include <asm/errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sstream>
#ifdef  CCXX_NAMESPACES
using namespace std;
using namespace ost;
#endif

#include "smsbbsprotocoldefine.h"

namespace SMS {

#define DB_NAME "AKA"
#define DB_HOST "localhost"
#define DB_USER "aka"
#define DB_PASSWORD "aA3$;G(~cjKK"

const char CODES[]="0123456789ABCDEFGHJKLMNPQRSTUVWXYZ";

/* {{{ class  CBBSChildProtocolTCPSocket
 * 程序的Server Socket实现
*/
class CBBSChildProtocolTCPSocket : public TCPSocket
{
	CSMSBBSChildPrivilegeChecker* m_privilegeChecker;
protected:
        bool onAccept(const InetHostAddress &ia, tpport_t port);

public:
        CBBSChildProtocolTCPSocket(InetAddress &ia, tpport_t port, CSMSBBSChildPrivilegeChecker* privilegeChecker);
};

CBBSChildProtocolTCPSocket::CBBSChildProtocolTCPSocket(InetAddress &ia, tpport_t port, CSMSBBSChildPrivilegeChecker* privilegeChecker) : 
	TCPSocket(ia, port),m_privilegeChecker(privilegeChecker) {};

bool CBBSChildProtocolTCPSocket::onAccept(const InetHostAddress &ia, tpport_t port){
		std::stringstream st;
		st<<ia;
		if (m_privilegeChecker->isConnectPermitted(st.str().c_str(),port)==TRUE) {
			syslog(LOG_ERR,"%s:%d's connection accepted",st.str().c_str(),port);
			return true;
		} else {
			syslog(LOG_ERR,"%s:%d's connection rejected",st.str().c_str(),port);
			return false;
		}
			
};
/* class CBBSChildProtocolTCPSocket
 * }}} */

/* {{{ class myTcpStream 
 * 程序的tcpstream实现
 */
class myTcpStream:public tcpstream{
public:
	ssize_t write(void* buf, ssize_t bufLen){
		return tcpstream::writeData(buf,bufLen);
	}

	ssize_t read(void* buf, ssize_t bufLen){
		return tcpstream::readData(buf,bufLen);
	}

};
/* class myTcpStream
* }}} */


class CSMSBBSChildProtocol: public CSMSProtocol{
	int m_pid;
	int m_state;
	myTcpStream *m_pStream;
	enum { ready,headLenghtUnkown, headIncomplete, bodyIncomplete };
	DWORD m_serial;
	CSMSBBSChildPrivilegeChecker *m_pChildPrivilegeChecker;
	CSMSStorage* m_pSMSStorage;
	char m_childCode[SMS_CHILDCODE_LEN+1];
	Connection m_conn;
	int m_listenPort;
private:

DWORD getSerial(){ //产生序列号
	return m_serial++;
}

/* {{{ doSendMsg()
 * 向下游发送消息
 */
int doSendMsg(void* msg, DWORD len){
	if (m_pStream==NULL){
		return ERROR;
	}
	//todo: 中断恢复与处理
	m_pStream->write(msg,len);
	return SUCCESS;
}
 /* doSendMsg()
  * }}} */

/* {{{ doReply()函数
 *
 * 返回消息处理结果
 */
int doReply(int msgType, const byte SerialNo[4], const byte pid[4]){
	char * msg;
	DWORD len;
	len=sizeof(SMS_BBS_HEADER);
	msg=(char *)malloc(len);
	memset(msg,0,len);
	((PSMS_BBS_HEADER)(msg))->Type=msgType;
	memcpy(((PSMS_BBS_HEADER)(msg))->SerialNo, SerialNo,4);
	memcpy(((PSMS_BBS_HEADER)(msg))->pid, pid, 4);
	sms_longToByte(((PSMS_BBS_HEADER)(msg))->msgLength,0);
	int retCode=doSendMsg(msg,len);
	delete[] msg;
	return retCode;
}
/* doRely() 
 *}}} */

/* {{{ convertSMS()
 * 转化SMS为内部格式
 */
int convertSMS(PSMS_BBS_BBSSENDSMS msg,  SMSMessage** sms, DWORD *smsLen){  

	*smsLen=sizeof(SMSMessage)+sms_byteToLong(msg->MsgTxtLen);
	*sms=(PSMSMessage) new char[*smsLen];
	if (*sms==NULL) {
		return NOENOUGHMEMORY;
	}

	memset(*sms,0,*smsLen);
	(*sms)->length=*smsLen;
	snprintf((*sms)->SenderNumber , MOBILENUMBERLENGTH , "%s%d%d" , m_childCode , SMS_BBS_TYPE_COMMON, sms_byteToLong(msg->UserID) );
	strncpy((*sms)->TargetNumber , msg->DstMobileNo , MOBILENUMBERLENGTH);
	strncpy((*sms)->FeeTargetNumber , msg->SrcMobileNo , MOBILENUMBERLENGTH);
	(*sms)->SMSBodyLength=sms_byteToLong(msg->MsgTxtLen);
	memcpy((*sms)->SMSBody, msg->MsgTxt, (*sms)->SMSBodyLength);

	return SUCCESS;
}

/* convertSMS()
 */

/* {{{ sendSMS()
 * 向上游发送短信
 *
 */
int sendSMS(PSMSMessage sms) {
	if (m_pSMSStorage->writeSMStoStorage(sms->SenderNumber,sms->TargetNumber,(char *)sms,sms->length)==SUCCESS) {
		return SMS_BBS_CMD_OK;
	} else {
		return SMS_BBS_CMD_ERR;
	}
}
/* sendSMS()
 * }}} */

/* {{{ generateValidateNum
 * 生成注册码
 *
 */
void generateValidateNum(char* validateNo, int validNumLen){
	int codeLen=strlen(CODES);
	srand(time(NULL));
	for (int i=0;i<validNumLen;i++){
		validateNo[i]=CODES[1+(int) (((double)codeLen)*rand()/(RAND_MAX+1.0))];
	}
	validateNo[validNumLen]=0;
}
/* generateValidateNum()
 * ))) */



/* {{{ getValidateNum()
 *  
 * 获取注册码
 */
int getValidateNum(const char* mobileNo, char* validateNo, int validNumLen){
	try {
		Query query=m_conn.query();
		query<< "select * from MobileRegisterNumber_TB where childCode='"<<m_childCode<<"' and targetMobileNo='"<<mobileNo<<"' ";
		Result res=query.store();
		if (res.size()!=0) {
			Row row=*(res.begin());
			if (strlen(row["ValidatationNumber"])!=0) {
				strncpy(validateNo,row["ValidatationNumber"],validNumLen);
				return SUCCESS;
			} else {
				generateValidateNum(validateNo,validNumLen);
				std::stringstream sql;
				sql<< "update  MobileRegisterNumber_TB set ValidatationNumber='"<<validateNo<<"' where childCode='"<<m_childCode<<"' and targetMobileNo='"<<mobileNo<<"' "; 
				query.exec(sql.str());
			}
		} else {
			generateValidateNum(validateNo,validNumLen);
			std::stringstream sql;
			sql<< "insert into MobileRegisterNumber_TB(childCode, MobilePhoneNumber, ValidatationNumber, RegisterCount) values( '" 
				<<m_childCode<<"' , '"<<mobileNo<<"' , '" <<validateNo <<"', 0 )";
			query.exec(sql.str());
		}
		return SUCCESS;
	} catch ( BadQuery er) {
		syslog(LOG_ERR," mysql query err : %s", er.error.c_str());
		return SMS_BBS_CMD_DB_ERROR;
	}
}
/* getValidateNum
 * }}} */

/* {{{ doSendRegisterSMS() 
 * 发送注册短信
 *
 */
int doSendRegisterSMS(const char* targetMobileNo){
	char validateNo[SMS_BBS_VALID_LEN+1];
	int retCode=getValidateNum(targetMobileNo,validateNo,SMS_BBS_VALID_LEN);
	if (retCode!=SUCCESS) {
		return retCode;
	}

	char msg[101];
	snprintf(msg, 100, "您的注册码是：%s", validateNo);
	DWORD smsLen=sizeof(SMSMessage)+sizeof(msg);
	PSMSMessage sms=(PSMSMessage)new char[smsLen];
	memset(sms,0,smsLen);
	sms->length=smsLen;

	snprintf(sms->SenderNumber , MOBILENUMBERLENGTH , "%s%d" , m_childCode , SMS_BBS_TYPE_REGISTER);

	strncpy(sms->TargetNumber , targetMobileNo , MOBILENUMBERLENGTH);
	strncpy(sms->FeeTargetNumber , targetMobileNo , MOBILENUMBERLENGTH);
	sms->SMSBodyLength=sizeof(msg);
	memcpy(sms->SMSBody, msg , sizeof(msg));
	retCode=sendSMS(sms);
	delete[] (char*)sms;
	return retCode;
}
/* doSendRegisterSMS()
 * }}} */

/* {{{ doRegisterValidation()
 * 使用注册码进行手机绑定认证
 */
int doRegisterValidation(const char* mobileNo, const char* validateNo){
	try {
		Query query=m_conn.query();
		query<< "select * from MobileRegisterNumber_TB where childCode='"<<m_childCode<<"' and targetMobileNo='"<<mobileNo<<"' ";
		Result res=query.store();
		if (res.size()!=0) {
			Row row=*(res.begin());
			if (strlen(row["ValidatationNumber"])!=0) {
				if (strcmp(validateNo,row["ValidatationNumber"])){
					return SMS_BBS_CMD_ERR;
				}
				int count=atoi(row["RegisterCount"])+1;
				std::stringstream sql;
				sql<< "update  MobileRegisterNumber_TB set ValidatationNumber='', RegisterCount="<<count<<" where childCode='"<<m_childCode<<"' and targetMobileNo='"<<mobileNo<<"' "; 
				query.exec(sql.str());
				return SMS_BBS_CMD_OK;
			} 
		} 
		return SMS_BBS_CMD_NO_VALIDCODE;
	} catch ( BadQuery er) {
		syslog(LOG_ERR," mysql query err : %s", er.error.c_str());
		return SMS_BBS_CMD_DB_ERROR;
	}
}
/* doRegisterValidation()
 * }}}  */

/* {{{ doUnregister()
 * 取消手机绑定
 */
int doUnregister(const char* mobileNo){
	try {
		Query query=m_conn.query();
		query<< "select * from MobileRegisterNumber_TB where childCode='"<<m_childCode<<"' and targetMobileNo='"<<mobileNo<<"' ";
		Result res=query.store();
		if (res.size()!=0) {
			Row row=*(res.begin());
			int count=atoi(row["RegisterCount"]);
			if (count!=0) {
				count--;
				std::stringstream sql;
				if (count || (strlen(row["ValidatationNumber"])!=0) ) {
					sql<< "update  MobileRegisterNumber_TB set ValidatationNumber='', RegisterCount="<<count<<" where childCode='"<<m_childCode<<"' and targetMobileNo='"<<mobileNo<<"' "; 
				} else {
					sql << "delete from MobileRegisterNumber_TB where childCode='"<<m_childCode<<"' and targetMobileNo='"<<mobileNo<<"' ";
				}
				query.exec(sql.str());
				return SMS_BBS_CMD_OK;
			} 
		} 
		return SMS_BBS_CMD_NO_VALIDCODE;
	} catch ( BadQuery er) {
		syslog(LOG_ERR," mysql query err : %s", er.error.c_str());
		return SMS_BBS_CMD_DB_ERROR;
	}
}
/* doUnregister()
 *   ))) */

/* {{{ doReplyRegisterRequest()
 * 绑定请求回应
 */
int doReplyRegisterRequest(const char* mobileNo, byte isSucceed, DWORD smsSerialNo) {
	//todo: 错误恢复和处理
	return SUCCESS;
}
/* doReplyRegisterRequest()
 */

/* doSend()
 * 处理SMS发送请求
 *
 */
int doSend(PSMS_BBS_BBSSENDSMS msg){
	if (!m_pChildPrivilegeChecker->canSendSMS(msg->SrcMobileNo,msg->DstMobileNo)){
		return SMS_BBS_CMD_SMS_VALIDATE_FAILED;
	}
	PSMSMessage sms;
	DWORD smsLen;
	if (convertSMS(msg,&sms,&smsLen)==NOENOUGHMEMORY) {
		syslog(LOG_ERR,"Fatal Error: no enough memory for SMS convertion!system exited!");
		exit(0);
	}
	int retCode=sendSMS(sms);
	delete[] (char*)sms;
	return retCode;
}
/* doSend()
 */

/* {{{ OnCMDOK()
 * 接收到回应成功消息时的处理函数
 */
int OnCMDOK(DWORD serialNo){
	//todo: 错误处理与恢复
	syslog(LOG_ERR," bbs send msg OK reply serial No is %d",serialNo);
	return SUCCESS;
}
/* OnCMDOK()
 * }}} */

/* {{{ OnCMDError()
 * 接收到回应失败消息时的处理函数
 */
int OnCMDError(DWORD serialNo){
	//todo: 错误处理与恢复
	syslog(LOG_ERR," bbs send msg Error reply serial No is %d",serialNo);
	return SUCCESS;
}
/* OnCMDError()
 * }}} */


/* {{{ dispatchMessage() 
 * 消息派发函数，将消息转化为具体的函数调用
 *
 */
int dispatchMessage( char* msg, DWORD len) {
	byte msgType;
	DWORD smsSerialNo, msgLen , pid;
	msgType=(PSMS_BBS_HEADER(msg))->Type;
	msgLen=sms_byteToLong((PSMS_BBS_HEADER(msg))->msgLength);
	smsSerialNo=sms_byteToLong((PSMS_BBS_HEADER(msg))->SerialNo);
	pid=sms_byteToLong((PSMS_BBS_HEADER(msg))->pid);
	if ( (msgLen+sizeof(SMS_BBS_HEADER))!=len) {
		doReply(SMS_BBS_CMD_LENGTH_ERR, (PSMS_BBS_HEADER(msg))->SerialNo,(PSMS_BBS_HEADER(msg))->pid);
		return -1;
	}

	int retCode=0;

	switch (msgType) {
		case SMS_BBS_CMD_LOGIN:
			syslog(LOG_ERR," bbs send login msg error! have logined! serial No is %d", smsSerialNo);
			doReply(SMS_BBS_CMD_ERR,(PSMS_BBS_HEADER(msg))->SerialNo,(PSMS_BBS_HEADER(msg))->pid);
			break;
		case SMS_BBS_CMD_LOGOUT:
			doReply(SMS_BBS_CMD_OK,(PSMS_BBS_HEADER(msg))->SerialNo,(PSMS_BBS_HEADER(msg))->pid);
			retCode=QUIT;
			break;
		case SMS_BBS_CMD_REG:
			doReply(doSendRegisterSMS((PSMS_BBS_REGISTERMOBILEPACKET(msg))->MobileNo),(PSMS_BBS_HEADER(msg))->SerialNo,(PSMS_BBS_HEADER(msg))->pid);
			break;
		case SMS_BBS_CMD_CHECK:
			doReply(doRegisterValidation((PSMS_BBS_REGISTERVALIDATIONPACKET(msg))->MobileNo,(PSMS_BBS_REGISTERVALIDATIONPACKET(msg))->ValidateNo),(PSMS_BBS_HEADER(msg))->SerialNo,(PSMS_BBS_HEADER(msg))->pid);
			break;
		case SMS_BBS_CMD_UNREG:
			doReply(doUnregister((PSMS_BBS_UNREGISTERMOBILEPACKET(msg))->MobileNo),(PSMS_BBS_HEADER(msg))->SerialNo,(PSMS_BBS_HEADER(msg))->pid);
			break;
		case SMS_BBS_CMD_REQUEST:
			syslog(LOG_ERR," bbs send REQUEST msg! error! serial No is %d",smsSerialNo);
			break;
		case SMS_BBS_CMD_REQUESTREPLY:
			doReplyRegisterRequest((PSMS_BBS_BINDREQUESTREPLYPACKET(msg))->MobileNo,(PSMS_BBS_BINDREQUESTREPLYPACKET(msg))->isSucceed, smsSerialNo);
			break;
		case SMS_BBS_CMD_BBSSEND:
			doReply(doSend((PSMS_BBS_BBSSENDSMS)msg),(PSMS_BBS_HEADER(msg))->SerialNo,(PSMS_BBS_HEADER(msg))->pid);
			break;
		case SMS_BBS_CMD_OK:
			OnCMDOK(smsSerialNo);
			break;
		case SMS_BBS_CMD_ERR:
			OnCMDError(smsSerialNo);
			break;
		default:
			syslog(LOG_ERR," bbs send msg type : %d serial No is %d",msgType, smsSerialNo);
			break;
	}
	return retCode;
}
/* dispatchMessage()
 */

/* {{{ OnAccept()
 *
 * 从客户端读取消息
 */
int OnAccept(myTcpStream* pStream){
	char buf[1000];
	int errCount=0;
	int i,ret,l;
	int len=0;
	byte msgType=0;

	l=sizeof(SMS_BBS_HEADER);
redo0:
	i=pStream->read(buf,l);
	if ((i<0) && (errno=EINTR)) {
		goto redo0;
	}
	len+=i;
	if (i<l) {
		syslog(LOG_ERR, "read login msg head error %d ", i);
		return -1;
	}
	msgType=(PSMS_BBS_HEADER(buf))->Type;

#ifdef DEBUG
	DWORD smsSerialNo, msgLen;
	msgLen=sms_byteToLong((PSMS_BBS_HEADER(buf))->msgLength);
	smsSerialNo=sms_byteToLong((PSMS_BBS_HEADER(buf))->SerialNo);
	syslog(LOG_ERR,"login msg head length %d",msgLen);
	syslog(LOG_ERR,"login msg sn %d",smsSerialNo);
#endif
	if ( (msgType!=SMS_BBS_CMD_LOGIN) ) {
		syslog(LOG_ERR, "login msg head type error");
		return -1;
	}
redo02:
	i=pStream->read(buf+len,msgLen);
	if ((i<0) && (errno=EINTR)) {
		goto redo02;
	}
	len+=i;
	if (i<msgLen) {
		syslog(LOG_ERR, "read msg head body error");
		return -1;
	}
	
	if (!m_pChildPrivilegeChecker->canUserConnect( (PSMS_BBS_LOGINPACKET(buf))->user,(PSMS_BBS_LOGINPACKET(buf))->password)  ){
		syslog(LOG_ERR,"connection user & password wrong!");
		return -1;
	}

	m_pStream=pStream;

	for (;;){
		len=0;
		l=sizeof(SMS_BBS_HEADER);
redo1:
		i=pStream->read(buf,l);
		if ((i<0) && (errno=EINTR)) {
			goto redo1;
		}

		len+=i;
		if (i<l) {
			syslog(LOG_ERR, "read msg head error %d ", i);
			break;
		}
#ifdef DEBUG
		msgType=(PSMS_BBS_HEADER(buf))->Type;
		msgLen=sms_byteToLong((PSMS_BBS_HEADER(buf))->msgLength);
		smsSerialNo=sms_byteToLong((PSMS_BBS_HEADER(buf))->SerialNo);
		syslog(LOG_ERR,"msg head length %d",msgLen);
		syslog(LOG_ERR,"msg sn %d",smsSerialNo);
#endif

redo2:
		i=pStream->read(buf+len,msgLen);
		if ((i<0) && (errno=EINTR)) {
			goto redo2;
		}

		len+=i;
		if (i<msgLen) {
			syslog(LOG_ERR, "read msg head body error");
			break;
		}
		if (dispatchMessage(buf,len)==QUIT) {
			return 0;
		}

	}


	m_pStream=NULL;
	return 0;
}
/*　OnAccept()
 * }}} */

/* {{{ getSMSType()
 * 获取短消息类别
 */
int getSMSType(const char * targetMobileNo) { 
	int prefixLen=8+SMS_CHILDCODE_LEN ;
	char type[2];
	if ( strlen(targetMobileNo)<=prefixLen){
		return SMS_BBS_TYPE_NONE;
	}
	type[0]=targetMobileNo[prefixLen];
	type[1]=0;
	return atoi(type);
}
/* getSMSType()
 * }}} */

/* {{{ getTargetID()
 * 获取短消息目标ID
 */
DWORD getTargetID(const char * targetMobileNo) {
	int prefixLen=8+SMS_CHILDCODE_LEN+SMS_BBS_TYPE_LEN;
	if (strlen(targetMobileNo)<=prefixLen){
		return 0L;
	}
	return atoi(targetMobileNo+(8+SMS_CHILDCODE_LEN+SMS_BBS_TYPE_LEN));
}
/* getTargetID()
 *  }}} */


/* {{{ deliverSMS()
 * 向下游传递短消息
 */
int deliverSMS(PSMSMessage msg) {

	PSMS_BBS_GWSENDSMS sms;


	DWORD smsLen=sizeof(SMS_BBS_GWSENDSMS)+msg->SMSBodyLength;
	
	syslog(LOG_ERR,"delivering msg ....");
	
	sms=(PSMS_BBS_GWSENDSMS)new char[smsLen];

	if (sms==NULL) {
		syslog(LOG_ERR,"Fatal Error: can't alloc enough memory for sms convertion!");
		exit(-1);
	}

	memset(sms,0,smsLen);
	sms->header.Type=SMS_BBS_CMD_GWSEND;
	sms_longToByte( (sms->header.SerialNo), getSerial());
	sms_longToByte( (sms->header.msgLength),smsLen-sizeof(SMS_BBS_HEADER) );

	sms_longToByte( sms->UserID, getTargetID(msg->TargetNumber) );

	strncpy(sms->SrcMobileNo, msg->SenderNumber , MOBILENUMBERLENGTH);

	sms_longToByte((sms->MsgTxtLen) , msg->SMSBodyLength);
	memcpy(sms->MsgTxt, msg->SMSBody, msg->SMSBodyLength);

	int retCode=doSendMsg(sms,smsLen);

	delete[] (char*)sms;

	return retCode;

}
/* deliverSMS()
 * ))) */

int doRegisterSMS(const char* mobileNo){
	try {
		Query query=m_conn.query();
		query<< "select * from MobileRegisterNumber_TB where childCode='"<<m_childCode<<"' and targetMobileNo='"<<mobileNo<<"' ";
		Result res=query.store();
		std::stringstream sql;

		if (res.size()!=0) {
			Row row=*(res.begin());
			int count=atoi(row["RegisterCount"])+1;
			sql<< "update  MobileRegisterNumber_TB set ValidatationNumber='', RegisterCount="<<count<<" where childCode='"<<m_childCode<<"' and targetMobileNo='"<<mobileNo<<"' "; 
		} else {
			sql<< "insert into MobileRegisterNumber_TB(childCode, MobilePhoneNumber, ValidatationNumber, RegisterCount) values( '" 
				<<m_childCode<<"' , '"<<mobileNo<<"' , '', 1 )";
		}
		query.exec(sql.str());
		return SUCCESS;
	} catch ( BadQuery er) {
		syslog(LOG_ERR," mysql query err : %s", er.error.c_str());
		return FAILED;
	}
}

int doUnregisterSMS(const char* mobileNo){
	try {
		Query query=m_conn.query();
		query<< "select * from MobileRegisterNumber_TB where childCode='"<<m_childCode<<"' and targetMobileNo='"<<mobileNo<<"' ";
		Result res=query.store();
		if (res.size()!=0) {
			Row row=*(res.begin());
			int count=atoi(row["RegisterCount"]);
			if (count!=0) {
				count--;
				std::stringstream sql;
				if (count || (strlen(row["ValidatationNumber"])!=0) ) {
					sql<< "update  MobileRegisterNumber_TB set ValidatationNumber='', RegisterCount="<<count<<" where childCode='"<<m_childCode<<"' and targetMobileNo='"<<mobileNo<<"' "; 
				} else {
					sql << "delete from MobileRegisterNumber_TB where childCode='"<<m_childCode<<"' and targetMobileNo='"<<mobileNo<<"' ";
				}
				query.exec(sql.str());
				return SUCCESS;
			} 
		} 
		return ERROR;
	} catch ( BadQuery er) {
		syslog(LOG_ERR," mysql query err : %s", er.error.c_str());
		return FAILED;
	}
}

/* {{{ processRegisterSMS()
 * 处理注册短消息
 */
int processRegisterSMS(PSMSMessage msg) {
	byte isRegister=0; //0 注册 , 1 注销

	int retCode;
	if (msg->SMSBodyLength<2) {
		syslog(LOG_ERR,"received error sms!");
		return ERROR;
	}

	if  ((msg->SMSBody[0]=='R') && (msg->SMSBody[1]=='G')) {
		retCode=doRegisterSMS(msg->SenderNumber);
	}else if  ((msg->SMSBody[0]=='U') && (msg->SMSBody[1]=='R')) {
		isRegister=1;
		retCode=doUnregisterSMS(msg->SenderNumber);
	}else {
		syslog(LOG_ERR,"received error sms!");
		return ERROR;
	}

	if (retCode!=SUCCESS) {
		return FAILED;
	}

	PSMS_BBS_BINDREQUESTPACKET sms;

	DWORD smsLen=sizeof(SMS_BBS_BINDREQUESTPACKET);
	
	sms=(PSMS_BBS_BINDREQUESTPACKET)new char[smsLen];

	if (sms==NULL) {
		syslog(LOG_ERR,"Fatal Error: can't alloc enough memory for message generation!");
		exit(-1);
	}

	memset(sms,0,smsLen);
	sms->header.Type=SMS_BBS_CMD_REQUEST;
	sms_longToByte( (sms->header.SerialNo), getSerial());
	sms_longToByte( (sms->header.msgLength),smsLen-sizeof(SMS_BBS_HEADER) );

	strncpy(sms->MobileNo, msg->SenderNumber , MOBILENUMBERLENGTH);

	sms->Bind=isRegister;
	retCode=doSendMsg(sms,smsLen);

	delete[] (char*)sms;

	return retCode;

}
/* processRegisterSMS();
 * }}} */
public:
/* {{{ 构造函数
 *	childCode: 网关子用户的代码
 *  password: 网关子用户的连接密码
 *  port: 监听端口
 * 
 */
	CSMSBBSChildProtocol(const char* childCode,const char* password,const char* addr, int port): m_conn(use_exceptions){
		m_pid=0;
		m_state=ready;
		m_pStream=NULL;
		m_serial=0;
		try {
			m_conn.connect(DB_NAME, DB_HOST, DB_USER, DB_PASSWORD);
		} catch (BadQuery er) {
			syslog(LOG_ERR," connect DB error: %s",er.error.c_str());
			exit(-1);
		}
		m_pChildPrivilegeChecker=new CSMSBBSChildPrivilegeChecker(childCode, password,addr,&m_conn);
		strncpy(m_childCode,childCode,SMS_CHILDCODE_LEN);
		m_listenPort=port;
	}
/* 构造函数 */
/* }}} */

/* {{{ Run()
 * 监听子用户连接
 * pSMSStorage SMSStorage接口，用于向上游发送短信
 */
	int Run(CSMSStorage* pSMSStorage){
		InetAddress addr;
		myTcpStream tcp;
		m_pSMSStorage=pSMSStorage;

        try   {
			CBBSChildProtocolTCPSocket* pServiceSocket=new CBBSChildProtocolTCPSocket(addr,m_listenPort,m_pChildPrivilegeChecker);

			while(pServiceSocket->isPendingConnection()){
				tcp.open(*pServiceSocket);
				if (!tcp) {
					continue;
				}
				if (m_pid!=0) {
					kill(m_pid,SIGTERM);
				}
				switch(m_pid=fork()){
					case 0:
						delete pServiceSocket;
						pSMSStorage->init();
						OnAccept(&tcp);
						tcp.close();
						exit(0);
						break;
					case -1:
						syslog(LOG_ERR,"fork error");
						exit(-1);
						break;
					default:
						tcp.close();
				}

			}	
		} catch (Socket *socket)
        {
                tpport_t port;
                InetAddress saddr = (InetAddress)socket->getPeer(&port);
				syslog(LOG_ERR,"socket error %s : %d", saddr.getHostname(), port);

                if(socket->getErrorNumber() == Socket::errResourceFailure)
                {
                        syslog(LOG_ERR, "bind failed; no resources" );
                }
                if(socket->getErrorNumber() == Socket::errBindingFailed)
                {
                        syslog(LOG_ERR, "bind failed; port busy" );
                }
        }
		return 0;
	}
/* Run() 
 * }}} */



/* {{{ Send()
 * 接收并处理上游过来的短信
 */
int Send(PSMSMessage msg){
	switch (getSMSType(msg->TargetNumber)) {
		case SMS_BBS_TYPE_COMMON:
			return deliverSMS(msg);
		case SMS_BBS_TYPE_REGISTER:
			return processRegisterSMS(msg);
		default:
			syslog(LOG_ERR," received unknown sms, targetNumber is : %s ", msg->TargetNumber);
			return FAILED;
	}

}
/* Send()
 * }}} */
	


	~CSMSBBSChildProtocol() {
	}
};

}

#endif
