/* smsbbschildprotocol
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
#include "smsmysqlfeecodegetter.h"
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
#include "smslogger.h"
#include "smstcpstream.h"

namespace SMS {

#define SEND_NO_CHECK	1
#define SEND_CHECK 0

const char CODES[]="0123456789";

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

class CSMSBBSChildProtocol: public CSMSProtocol{
	int m_pid;
	int m_state;
	CSMSTcpStream *m_pStream;
	enum { ready,headLenghtUnkown, headIncomplete, bodyIncomplete };
	DWORD m_serial;
	CSMSBBSChildPrivilegeChecker *m_pChildPrivilegeChecker;
	CSMSStorage* m_pSMSStorage;
	char m_childCode[SMS_MAXCHILDCODE_LEN+1];
	Connection m_conn;
	int m_listenPort;
	CSMSLogger m_SMSLogger;
	int m_defaultMoneyLimit;
	CSMSFeeCodeGetter* m_pSMSFeeCodeGetter;
private:

DWORD getSerial(){ //产生序列号
	return m_serial++;
}

/* {{{ doSendMsg()
 * 向下游发送消息
 */
int doSendMsg(void* msg, DWORD len){
	int rc=0;
	if (m_pStream==NULL){
		return ERROR;
	}
	syslog(LOG_ERR," send msg to child ,length=%d",len);
	//todo: 中断恢复与处理

	rc=m_pStream->write((char*)msg,len);
	if ( rc<len) {
			syslog(LOG_ERR, "do send msg error: %d" , errno);
			return ERROR;
	}
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
	free( msg);
	return retCode;
}
/* doRely() 
 *}}} */

/* {{{ generateSMS()
 * 生成内部格式SMS
 */
int generateSMS(DWORD userCode, const char* targetNumber, const char* feeTargetNumber, const char* msgContent, DWORD msgLen , byte feeType , SMSMessage** sms, DWORD *smsLen){  
	*smsLen=sizeof(SMSMessage)+msgLen;
	*sms=(PSMSMessage) malloc(*smsLen);
	if (*sms==NULL) {
		return NOENOUGHMEMORY;
	}
	memset(*sms,0,*smsLen);
	(*sms)->length=*smsLen;

	if (userCode) 
		snprintf((*sms)->SenderNumber , MOBILENUMBERLENGTH , "%s%d" , m_childCode, userCode);
	else 
		snprintf((*sms)->SenderNumber , MOBILENUMBERLENGTH , "%s" , m_childCode);

	strncpy((*sms)->TargetNumber , targetNumber , MOBILENUMBERLENGTH);
	(*sms)->TargetNumber[MOBILENUMBERLENGTH]=0;
	strncpy((*sms)->FeeTargetNumber , feeTargetNumber , MOBILENUMBERLENGTH);
	(*sms)->FeeTargetNumber[MOBILENUMBERLENGTH]=0;
	(*sms)->SMSBodyLength=msgLen;
	memcpy((*sms)->SMSBody, msgContent, msgLen);

	(*sms)->sendTime=time(NULL);

	strncpy((*sms)->childCode,m_childCode,SMS_MAXCHILDCODE_LEN);
	(*sms)->childCode[SMS_MAXCHILDCODE_LEN]=0;

	(*sms)->FeeType=feeType;

	return SUCCESS;
}

/* generateSMS()
 * }}} */

/* {{{ convertSMS()
 * 转化SMS为内部格式
 */
int convertSMS(PSMS_BBS_BBSSENDSMS msg,  SMSMessage** sms, DWORD *smsLen){  
	return generateSMS(sms_byteToLong(msg->UserID),msg->DstMobileNo, msg->SrcMobileNo,msg->MsgTxt,sms_byteToLong(msg->MsgTxtLen),6, sms,smsLen);
}

/* convertSMS()
 *  }}} */

int countMoney(const char* mobileNumber, const char* usrID, int feeMoney) {
	try{
			Query query=m_conn.query();
			query<< "select * from SMSRegister_TB where childCode='"<<m_childCode<<"' and MobilePhoneNumber='"<<mobileNumber<<"' and UPPER(srcID)=UPPER('"<<usrID<<"') and todayMoney+"<<feeMoney<<"<=moneyLimit";
			Result res=query.store();
			if (res.size()!=0) {
					std::stringstream sql;
					sql<< "update  SMSRegister_TB set todayMoney=todayMoney+"<<feeMoney<<" where childCode='"<<m_childCode<<"' and MobilePhoneNumber='"<<mobileNumber<<"' and UPPER(srcID)=UPPER('"<<usrID<<"') ";
					query.exec(sql.str());			
					return SMS_BBS_CMD_OK;
			} else {
				syslog(LOG_ERR,"%s %s 's todayMoneyLimit exeeded", usrID, mobileNumber);
				return SMS_BBS_CMD_EXCEEDMONEY_LIMIT;
			}
	} catch ( BadQuery er) {
			syslog(LOG_ERR," mysql query err : %s", er.error.c_str());
	}
	return SMS_BBS_CMD_DB_ERROR;
}

/* {{{ sendSMS()
 * 向上游发送短信
 *
 */
int sendSMS(PSMSMessage sms,const char* usrID, int bCheck=SEND_CHECK) {
	if (bCheck!=SEND_NO_CHECK) {
		int feeMoney=0;
		int retCode;
		if (m_pSMSFeeCodeGetter->getFee(sms->FeeType,&feeMoney)!=SUCCESS){
			return SMS_BBS_CMD_DB_ERROR;
		}
		if ((retCode=countMoney(sms->FeeTargetNumber,usrID,feeMoney))!=SMS_BBS_CMD_OK) {
			return retCode;
		}
	}
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
	int t;
	srand(time(NULL));
	syslog(LOG_ERR," %d %d ", codeLen, validNumLen);
	for (int i=0;i<validNumLen;i++){
		t=0+(int) (((double)codeLen)*rand()/(RAND_MAX+1.0));
		validateNo[i]=CODES[t];
		syslog(LOG_ERR,"%c",validateNo[i]);
	}
	validateNo[validNumLen]=0;
}
/* generateValidateNum()
 * ))) */



/* {{{ getValidateNum()
 *  
 * 获取注册码
 */
int getValidateNum(const char* mobileNo, const char* srcID, char* validateNo, int validNumLen){
	try {
		Query query=m_conn.query();
		query<< "select * from SMSRegister_TB where childCode='"<<m_childCode<<"' and MobilePhoneNumber='"<<mobileNo<<"' and UPPER(srcID)=UPPER('"<<srcID<<"')";
		Result res=query.store();
		if (res.size()!=0) {
			Row row=*(res.begin());
			if (strlen(row["ValidatationNumber"])!=0) {
				strncpy(validateNo,row["ValidatationNumber"],validNumLen);
				validateNo[validNumLen]=0;
				return SUCCESS;
			} 
		}
		generateValidateNum(validateNo,validNumLen);
		std::stringstream sql;
		sql<< "replace into SMSRegister_TB(childCode, MobilePhoneNumber, ValidatationNumber,srcID,moneyLimit) values( '" 
				<<m_childCode<<"' , '"<<mobileNo<<"' , '" <<validateNo <<"', '"<<srcID<<"',"<<m_defaultMoneyLimit<< " )";
		query.exec(sql.str());
		return SUCCESS;
	} catch ( BadQuery er) {
		syslog(LOG_ERR," getValidateNum -- mysql query err : %s", er.error.c_str());
		return SMS_BBS_CMD_DB_ERROR;
	}
}
/* getValidateNum
 * }}} */

/* {{{ doSendRegisterSMS() 
 * 发送注册短信
 *
 */
int doSendRegisterSMS(const char* targetMobileNo, const char* usrID){
	char validateNo[SMS_BBS_VALID_LEN+1];
	int retCode=getValidateNum(targetMobileNo,usrID, validateNo,SMS_BBS_VALID_LEN);
	if (retCode!=SUCCESS) {
		return retCode;
	}


	PSMSMessage sms;
	DWORD smsLen;
	char msg[101];
	snprintf(msg, 100, "您的注册码是：%s", validateNo);
	if (generateSMS(0,targetMobileNo, targetMobileNo,msg,strlen(msg),6, &sms,&smsLen)==NOENOUGHMEMORY) {
		syslog(LOG_ERR,"Fatal Error: no enough memory for SMS convertion!system exited!");
		exit(0);
	}
	retCode=sendSMS(sms,usrID);
	free(sms);
	return retCode;
}
/* doSendRegisterSMS()
 * }}} */

/* {{{ doRegisterValidation()
 * 使用注册码进行手机绑定认证
 */
int doRegisterValidation(const char* mobileNo, const char* srcID, const char* validateNo){
	try {
		Query query=m_conn.query();
		query<< "select * from SMSRegister_TB where childCode='"<<m_childCode<<"' and MobilePhoneNumber='"<<mobileNo<<"' and UPPER(srcID)=UPPER('"<<srcID<<"') ";
		Result res=query.store();
		if (res.size()!=0) {
			Row row=*(res.begin());
			if (strlen(row["ValidatationNumber"])!=0) {
					if (strcmp(validateNo,row["ValidatationNumber"])){
							return SMS_BBS_CMD_ERR;
					}
					std::stringstream sql;
					sql<< "update  SMSRegister_TB set ValidatationNumber='' where childCode='"<<m_childCode<<"' and MobilePhoneNumber='"<<mobileNo<<"' and UPPER(srcID)=UPPER('"<<srcID<<"') ";
					query.exec(sql.str());
					return SMS_BBS_CMD_OK;
			}
		} 
		return SMS_BBS_CMD_NO_VALIDCODE;
	} catch ( BadQuery er) {
		syslog(LOG_ERR,"doRegisterValidation --  mysql query err : %s", er.error.c_str());
		return SMS_BBS_CMD_DB_ERROR;
	}
}
/* doRegisterValidation()
 * }}}  */

/* {{{ doUnregister()
 * 取消手机绑定
 */
int doUnregister(const char* mobileNo,const char* srcID){
	try {
		Query query=m_conn.query();
		query<< "select * from SMSRegister_TB where childCode='"<<m_childCode<<"' and MobilePhoneNumber='"<<mobileNo<<"' and UPPER(srcID)=UPPER('"<<srcID<<"') ";
		Result res=query.store();
		if (res.size()!=0) {
			std::stringstream sql;
			sql << "delete from SMSRegister_TB where childCode='"<<m_childCode<<"' and MobilePhoneNumber='"<<mobileNo<<"' and UPPER(srcID)=UPPER('"<<srcID<<"') ";
			query.exec(sql.str());
			return SMS_BBS_CMD_OK;
		} 
		return SMS_BBS_CMD_OK;
	} catch ( BadQuery er) {
		syslog(LOG_ERR,"doUnregister -- mysql query err : %s", er.error.c_str());
		return SMS_BBS_CMD_DB_ERROR;
	}
}
/* doUnregister()
 *   ))) */

/* {{{ doReplyRegisterRequest()
 * 绑定请求回应
 */
int doReplyRegisterRequest(const char* mobileNo, byte isSucceed, DWORD smsSerialNo) {
	//todo: 错误处理与恢复
	return SUCCESS;
}
/* doReplyRegisterRequest()
 */

/* doSend()
 * 处理SMS发送请求
 *
 */
int doSend(PSMS_BBS_BBSSENDSMS msg){
	if (!m_pChildPrivilegeChecker->canSendSMS(msg->SrcMobileNo,msg->srcUserID)){
		return SMS_BBS_CMD_SMS_VALIDATE_FAILED;
	}
	PSMSMessage sms;
	DWORD smsLen;
	int retCode;
	if (convertSMS(msg,&sms,&smsLen)==NOENOUGHMEMORY) {
		syslog(LOG_ERR,"Fatal Error: no enough memory for SMS convertion!system exited!");
		exit(0);
	}
	retCode=sendSMS(sms,msg->srcUserID);
	free(sms);
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
		return QUIT;
	}

	int retCode=0;

#ifdef DEBUG
	syslog(LOG_ERR,"dispatching message...");
	syslog(LOG_ERR," msg type %d",msgType);
	syslog(LOG_ERR," msg sn %d",smsSerialNo);
#endif

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
			doReply(doSendRegisterSMS((PSMS_BBS_REGISTERMOBILEPACKET(msg))->MobileNo,(PSMS_BBS_REGISTERMOBILEPACKET(msg))->cUserID),(PSMS_BBS_HEADER(msg))->SerialNo,(PSMS_BBS_HEADER(msg))->pid);
			break;
		case SMS_BBS_CMD_CHECK:
			doReply(doRegisterValidation((PSMS_BBS_REGISTERVALIDATIONPACKET(msg))->MobileNo,(PSMS_BBS_REGISTERVALIDATIONPACKET(msg))->cUserID, (PSMS_BBS_REGISTERVALIDATIONPACKET(msg))->ValidateNo),(PSMS_BBS_HEADER(msg))->SerialNo,(PSMS_BBS_HEADER(msg))->pid);
			break;
		case SMS_BBS_CMD_UNREG:
			doReply(doUnregister((PSMS_BBS_UNREGISTERMOBILEPACKET(msg))->MobileNo,(PSMS_BBS_UNREGISTERMOBILEPACKET(msg))->cUserID),(PSMS_BBS_HEADER(msg))->SerialNo,(PSMS_BBS_HEADER(msg))->pid);
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
			return QUIT;
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
int OnAccept(CSMSTcpStream* pStream){
	char buf[1000];
	int errCount=0;
	int i,ret,l;
	int len=0;
	byte msgType=0;

	l=sizeof(SMS_BBS_HEADER);
	i=pStream->read(buf+len,l);
	if (i<l) {
		syslog(LOG_ERR," read login msg header error %d!", errno);
		return -1;
	}
	len+=i;
	msgType=(PSMS_BBS_HEADER(buf))->Type;


#ifdef DEBUG
	DWORD smsSerialNo, msgLen ;
	msgLen=sms_byteToLong((PSMS_BBS_HEADER(buf))->msgLength);
	smsSerialNo=sms_byteToLong((PSMS_BBS_HEADER(buf))->SerialNo);
	syslog(LOG_ERR,"login msg head length %d",msgLen);
	syslog(LOG_ERR,"login msg sn %d",smsSerialNo);
#endif
	if ( (msgType!=SMS_BBS_CMD_LOGIN) ) {
		syslog(LOG_ERR, "login msg head type error");
		return -1;
	}
	i=pStream->read(buf+len,msgLen);
	if (i<l) {
		syslog(LOG_ERR," read login msg header body error %d!", errno);
		return -1;
	}
	len+=i;
	syslog(LOG_ERR," %s login %s", (PSMS_BBS_LOGINPACKET(buf))->user,(PSMS_BBS_LOGINPACKET(buf))->password);

	
	if (!m_pChildPrivilegeChecker->canUserConnect( (PSMS_BBS_LOGINPACKET(buf))->user,(PSMS_BBS_LOGINPACKET(buf))->password)  ){
		doReply(SMS_BBS_CMD_ERR,(PSMS_BBS_HEADER(buf))->SerialNo,(PSMS_BBS_HEADER(buf))->pid);
		syslog(LOG_ERR,"connection user & password wrong!");
		return -1;
	}
	m_pStream=pStream;
	m_pSMSStorage->init();
	m_pSMSStorage->OnNotify();
	doReply(SMS_BBS_CMD_OK,(PSMS_BBS_HEADER(buf))->SerialNo,(PSMS_BBS_HEADER(buf))->pid);

	for (;;){
		len=0;
		l=sizeof(SMS_BBS_HEADER);
		i=pStream->read(buf,l);
		if (i<l) {
			syslog(LOG_ERR," read msg header error %d!", errno);
			return -1;
		}

		len+=i;
#ifdef DEBUG
		msgType=(PSMS_BBS_HEADER(buf))->Type;
#endif
		msgLen=sms_byteToLong((PSMS_BBS_HEADER(buf))->msgLength);
#ifdef DEBUG
		smsSerialNo=sms_byteToLong((PSMS_BBS_HEADER(buf))->SerialNo);
		{
			char sbuf[5000];
			for (i=0;i<l;i++){
				sprintf(sbuf+i*3, "%02X ",buf[i]);
			}
			syslog(LOG_ERR,"msg header: %s",sbuf);
		}
		syslog(LOG_ERR,"msg head length %d",msgLen);
		syslog(LOG_ERR,"msg sn %d",smsSerialNo);
#endif

		i=pStream->read(buf+len,msgLen);
		if (i<msgLen) {
			syslog(LOG_ERR," read msg header error %d!", errno);
			return -1;
		}

		len+=i;
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
	int prefixLen=strlen(m_childCode);
	if ( strlen(targetMobileNo)<prefixLen){
		return SMS_BBS_TYPE_NONE;
	}
	if ( strlen(targetMobileNo)==prefixLen){
                return SMS_BBS_TYPE_COMMAND;
	}
	return SMS_BBS_TYPE_COMMON;
}
/* getSMSType()
 * }}} */

/* {{{ getTargetID()
 * 获取短消息目标ID
 */
DWORD getTargetID(const char * targetMobileNo) {
	int prefixLen=strlen(m_childCode);
	if (strlen(targetMobileNo)<=prefixLen){
		return 0L;
	}
	return atoi(targetMobileNo+(prefixLen));
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
	
	sms=(PSMS_BBS_GWSENDSMS)malloc(smsLen);

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

	syslog(LOG_ERR,"deliver sms to child, retCode=%d",retCode);


	free(sms);

	return retCode;

}
/* deliverSMS()
 * ))) */

int doRegisterSMS(const char* mobileNo,const char* srcID){
	try {
		Query query=m_conn.query();
		query<< "select * from SMSRegister_TB where childCode='"<<m_childCode<<"' and MobilePhoneNumber='"<<mobileNo<<"' and UPPER(srcID)=UPPER('"<<srcID<<"') ";
		Result res=query.store();
		syslog(LOG_ERR,"%s %s %s",m_childCode,mobileNo,srcID);

		if (res.size()!=0) {
			return NOSEVEREERROR;
		} else {
			std::stringstream sql;
			sql<< "insert into SMSRegister_TB(childCode, MobilePhoneNumber, ValidatationNumber, srcID, moneyLimit) values( '" 
				<<m_childCode<<"' , '"<<mobileNo<<"' , '', '"<<srcID<<"',"<<m_defaultMoneyLimit<<" )";
			query.exec(sql.str());
		}
		return SUCCESS;
	} catch ( BadQuery er) {
		syslog(LOG_ERR,"doRegisterSMS -- mysql query err : %s", er.error.c_str());
		return FAILED;
	}
}

int doUnregisterSMS(const char* mobileNo,const char* srcID){
	try {
		Query query=m_conn.query();
		query<< "select * from SMSRegister_TB where childCode='"<<m_childCode<<"' and MobilePhoneNumber='"<<mobileNo<<"' and UPPER(srcID)=UPPER('"<<srcID<<"') ";
		Result res=query.store();
		if (res.size()!=0) {
			std::stringstream sql;
			sql << "delete from SMSRegister_TB where childCode='"<<m_childCode<<"' and MobilePhoneNumber='"<<mobileNo<<"' and UPPER(srcID)=UPPER('"<<srcID<<"') ";
			query.exec(sql.str());
			return SUCCESS;
		} 
		return NOSEVEREERROR;
	} catch ( BadQuery er) {
		syslog(LOG_ERR,"doUnregisterSMS -- mysql query err : %s", er.error.c_str());
		return FAILED;
	}
}

int doSendRegisterMsg(const char* mobileNumber, const char * usrID, byte isBind) {
	SMS_BBS_BINDREQUESTPACKET sms;
	DWORD smsLen=sizeof(SMS_BBS_BINDREQUESTPACKET);
	
	memset(&sms,0,smsLen);
	sms.header.Type=SMS_BBS_CMD_REQUEST;
	sms_longToByte( (sms.header.SerialNo), getSerial());
	sms_longToByte( (sms.header.msgLength),smsLen-sizeof(SMS_BBS_HEADER) );

	strncpy(sms.MobileNo, mobileNumber, MOBILENUMBERLENGTH);
	sms.MobileNo[MOBILENUMBERLENGTH]=0;
	strncpy(sms.cUserID, usrID, SMS_BBS_ID_LEN);

	sms.cUserID[SMS_BBS_ID_LEN]=0;

	sms.Bind=isBind;
	int retCode=doSendMsg(&sms,smsLen);

	return retCode;
}

int doRegisterCommand(const char* mobileNumber, const char * usrID) {
	int retCode=doRegisterSMS(mobileNumber,usrID);
	if (retCode==ERROR) { 
		return retCode;
	}
	PSMSMessage sms;
	DWORD smsLen;
	char msg[101];
	if (retCode==NOSEVEREERROR) {
		snprintf(msg, 100, "您的手机号与id:%s已处在绑定状态,请不要重复绑定.", usrID);
	} else {
		snprintf(msg, 100, "您的手机号与id:%s已成功绑定.",usrID);
	}
		
	if (generateSMS(0,mobileNumber, mobileNumber,msg,strlen(msg),6, &sms,&smsLen)==NOENOUGHMEMORY) {
		syslog(LOG_ERR,"Fatal Error: no enough memory for SMS convertion!system exited!");
		exit(0);
	}
	retCode=(sendSMS(sms,usrID)==SMS_BBS_CMD_OK)?SUCCESS:ERROR;
	free(sms);
	if (retCode!=SUCCESS){
		return ERROR;
	}
	return doSendRegisterMsg(mobileNumber,usrID, SMS_BBS_USR_REQUIRE_UNBIND);
}
int doUnRegisterCommand(const char* mobileNumber, const char * usrID) {
	int retCode=doUnregisterSMS(mobileNumber,usrID);
	if (retCode==ERROR) { 
		return retCode;
	}
	PSMSMessage sms;
	DWORD smsLen;
	char msg[101];
	if (retCode==NOSEVEREERROR) {
		snprintf(msg, 100, "您的手机号与id:%s并没有绑定.", usrID);
	} else {
		snprintf(msg, 100, "您的手机号与id:%s已成功解除绑定.", usrID);
	}
		
	if (generateSMS(0,mobileNumber, mobileNumber,msg,strlen(msg),6, &sms,&smsLen)==NOENOUGHMEMORY) {
		syslog(LOG_ERR,"Fatal Error: no enough memory for SMS convertion!system exited!");
		exit(0);
	}
	retCode=(sendSMS(sms,usrID,SEND_NO_CHECK)==SMS_BBS_CMD_OK)?SUCCESS:ERROR;
	free(sms);
	if (retCode!=SUCCESS){
		return ERROR;
	}
	return doSendRegisterMsg(mobileNumber,usrID, SMS_BBS_USR_REQUIRE_BIND);
}

int doSetMoneyLimit(const char* mobileNumber, const char * usrID, int limit){
	try{
		Query query=m_conn.query();
		std::stringstream sql;
		sql<< "update  SMSRegister_TB set moneyLimit="<<limit<<" where childCode='"<<m_childCode<<"' and MobilePhoneNumber='"<<mobileNumber<<"' and UPPER(srcID)=UPPER('"<<usrID<<"') ";
		query.exec(sql.str());			
		return SUCCESS;
	} catch ( BadQuery er) {
		syslog(LOG_ERR," mysql query err : %s", er.error.c_str());
	}
	return ERROR;
}

int doSetMoneyLimitCommand(const char* mobileNumber, const char * usrID, int limit) {
	int retCode=doSetMoneyLimit(mobileNumber,usrID,limit);
	if (retCode!=SUCCESS) {
		return retCode;
	}
	PSMSMessage sms;
	DWORD smsLen;
	char msg[101];
	snprintf(msg, 100, "您已成功设置bbs每日短信发送限额为 %d.%02d 元", limit/100, limit%100);
	if (generateSMS(0,mobileNumber, mobileNumber,msg,strlen(msg),6, &sms,&smsLen)==NOENOUGHMEMORY) {
		syslog(LOG_ERR,"Fatal Error: no enough memory for SMS convertion!system exited!");
		exit(0);
	}
	retCode=(sendSMS(sms,usrID)==SMS_BBS_CMD_OK)?SUCCESS:ERROR;
	free(sms);
	return retCode;
}

int doZeroTodayTotal(const char* mobileNumber, const char * usrID){
	try{
		Query query=m_conn.query();
		std::stringstream sql;
		sql<< "update  SMSRegister_TB set todayMoney=0 where childCode='"<<m_childCode<<"' and MobilePhoneNumber='"<<mobileNumber<<"' and UPPER(srcID)=UPPER('"<<usrID<<"') ";
		query.exec(sql.str());			
		return SUCCESS;
	} catch ( BadQuery er) {
		syslog(LOG_ERR," mysql query err : %s", er.error.c_str());
	}
	return ERROR;
}

int doZeroTodayTotalCommand(const char* mobileNumber, const char * usrID) {
	int retCode=doZeroTodayTotal(mobileNumber,usrID);
	if (retCode!=SUCCESS) {
		return retCode;
	}
	PSMSMessage sms;
	DWORD smsLen;
	char msg[101];
	snprintf(msg, 100, "您已成功清零本日bbs短信累积发送金额");
	if (generateSMS(0,mobileNumber, mobileNumber,msg,strlen(msg),6, &sms,&smsLen)==NOENOUGHMEMORY) {
		syslog(LOG_ERR,"Fatal Error: no enough memory for SMS convertion!system exited!");
		exit(0);
	}
	retCode=(sendSMS(sms,usrID)==SMS_BBS_CMD_OK)?SUCCESS:ERROR;
	free(sms);
	return retCode;
}

int doGetMoneyLimit(const char* mobileNumber, const char * usrID, int* pLimit){
	try{
		Query query=m_conn.query();
		query<< "select moneyLimit  from SMSRegister_TB where childCode='"<<m_childCode<<"' and MobilePhoneNumber='"<<mobileNumber<<"' and UPPER(srcID)=UPPER('"<<usrID<<"') ";
		Result res=query.store();
		if (res.size()!=0) {
			Row row=*(res.begin());
			*pLimit=atoi(row["moneyLimit"]);
			return SUCCESS;
		} 
	} catch ( BadQuery er) {
		syslog(LOG_ERR," mysql query err : %s", er.error.c_str());
	}
	return FAILED;
}

int doGetMoneyLimitCommand(const char* mobileNumber, const char * usrID) {
	int limit;
	int retCode=doGetMoneyLimit(mobileNumber,usrID,&limit);
	if (retCode!=SUCCESS) {
		return retCode;
	}
	PSMSMessage sms;
	DWORD smsLen;
	char msg[101];
	snprintf(msg, 100, "您当前的bbs日短信发送限额为 %d.%02d 元", limit/100, limit %100);
	if (generateSMS(0,mobileNumber, mobileNumber,msg,strlen(msg),6, &sms,&smsLen)==NOENOUGHMEMORY) {
		syslog(LOG_ERR,"Fatal Error: no enough memory for SMS convertion!system exited!");
		exit(0);
	}
	retCode=(sendSMS(sms,usrID)==SMS_BBS_CMD_OK)?SUCCESS:ERROR;
	free(sms);
	return retCode;
}

int doGetTodayTotal(const char* mobileNumber, const char * usrID, int* pTotal){
	try{
		Query query=m_conn.query();
		query<< "select todayMoney  from SMSRegister_TB where childCode='"<<m_childCode<<"' and MobilePhoneNumber='"<<mobileNumber<<"' and UPPER(srcID)=UPPER('"<<usrID<<"') ";
		Result res=query.store();
		if (res.size()!=0) {
			Row row=*(res.begin());
			*pTotal=atoi(row["todayMoney"]);
			return SUCCESS;
		} 
	} catch ( BadQuery er) {
		syslog(LOG_ERR," mysql query err : %s", er.error.c_str());
	}
	return FAILED;
}

int doGetTodayTotalCommand(const char* mobileNumber, const char * usrID) {
	int total;
	int retCode=doGetTodayTotal(mobileNumber,usrID,&total);
	if (retCode!=SUCCESS) {
		return retCode;
	}
	PSMSMessage sms;
	DWORD smsLen;
	char msg[101];
	snprintf(msg, 100, "您本日在bbs上已收发短信共 %d.%02d 元", total/100, total%100);
	if (generateSMS(0,mobileNumber, mobileNumber,msg,strlen(msg),6, &sms,&smsLen)==NOENOUGHMEMORY) {
		syslog(LOG_ERR,"Fatal Error: no enough memory for SMS convertion!system exited!");
		exit(0);
	}
	retCode=(sendSMS(sms,usrID)==SMS_BBS_CMD_OK)?SUCCESS:ERROR;
	free(sms);
	return retCode;
}

/* {{{ processRegisterSMS()
 * 处理用户命令短消息
 */
int processCommandSMS(PSMSMessage msg) {
	char usrID[SMS_BBS_MAX_COMMAND_SMS_LEN+1]="";
	char command[SMS_BBS_MAX_COMMAND_SMS_LEN+1]="";
	char option1[SMS_BBS_MAX_COMMAND_SMS_LEN+1]="";
        char option2[SMS_BBS_MAX_COMMAND_SMS_LEN+1]="";	
	char buf[SMS_BBS_MAX_COMMAND_SMS_LEN+1];
	int retCode;
	if ( (msg->SMSBodyLength>SMS_BBS_MAX_COMMAND_SMS_LEN)){
		syslog(LOG_ERR,"received error registe sms!");
		return ERROR;
	}
	memcpy(buf,msg->SMSBody,msg->SMSBodyLength);
	buf[msg->SMSBodyLength]=0;
	sscanf(buf,"%s %s %s %s",command, usrID, option1,option2);
#ifdef DEBUG
	syslog(LOG_ERR,"command: -%s- srcID: -%s- option1: -%s- option2: -%s-",command,usrID,option1,option2);
#endif
	if (strlen(usrID)==0)
		return PARSE_ERROR;
	if (!strcasecmp(command,"ZCYH")) { //上行注册
		return doRegisterCommand(msg->SenderNumber, usrID);
	} 
	if (!strcasecmp(command,"QXZC")) { //取消注册
		return doUnRegisterCommand(msg->SenderNumber, usrID);
	} 
	if (!strcasecmp(command,"SZXE")) { //设置限额
		return doSetMoneyLimitCommand(msg->SenderNumber, usrID,atoi(option1));
	}
	if (!strcasecmp(command,"QKYY")) { //清空当日已用钱数
		return doZeroTodayTotalCommand(msg->SenderNumber, usrID);
	}
	if (!strcasecmp(command,"CXXE")) { //查询限额
		return doGetMoneyLimitCommand(msg->SenderNumber , usrID);
	}
	if (!strcasecmp(command,"CXYY")) { //查询当日已用钱数
		return doGetTodayTotalCommand(msg->SenderNumber , usrID);
	}

	return PARSE_ERROR;


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
	CSMSBBSChildProtocol(const char* childCode,const char* password,const char* addr, int port,int defaultMoneyLimit): m_conn(use_exceptions),m_SMSLogger(&m_conn){
		m_pid=0;
		m_state=ready;
		m_pStream=NULL;
		m_serial=0;
		m_pChildPrivilegeChecker=new CSMSBBSChildPrivilegeChecker(childCode, password,addr,&m_conn);
		strncpy(m_childCode,childCode,SMS_MAXCHILDCODE_LEN);
		m_childCode[SMS_MAXCHILDCODE_LEN]=0;
		m_listenPort=port;
		m_defaultMoneyLimit=defaultMoneyLimit;
		m_pSMSFeeCodeGetter=new CSMSMysqlFeeCodeGetter(&m_conn);
		
	}
/* 构造函数 */
/* }}} */

/* {{{ Run()
 * 监听子用户连接
 * pSMSStorage SMSStorage接口，用于向上游发送短信
 */
	int Run(CSMSStorage* pSMSStorage){
		InetAddress addr;
		CSMSTcpStream tcp;
		m_pSMSStorage=pSMSStorage;

        try   {
			CBBSChildProtocolTCPSocket* pServiceSocket=new CBBSChildProtocolTCPSocket(addr,m_listenPort,m_pChildPrivilegeChecker);

			while(pServiceSocket->isPendingConnection()){
				tcp.open(*pServiceSocket);
				if (!tcp) {
					continue;
				}
				if (m_pid!=0) {
			//		kill(m_pid,SIGTERM);
					kill(m_pid,SIGKILL);
				}
				switch(m_pid=fork()){
					case 0:
						delete pServiceSocket;
						try {
							m_conn.connect(DB_NAME, DB_HOST, DB_USER, DB_PASSWORD);
						} catch (BadQuery er) {
							syslog(LOG_ERR," connect DB error: %s",er.error.c_str());
							exit(-1);
						}
						OnAccept(&tcp);
						tcp.close();
						m_conn.close();
						exit(0);
						break;
					case -1:
						syslog(LOG_ERR,"fork error");
						tcp.close();
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
	int smsType;
	int retCode;
	syslog(LOG_ERR, "sending msg...%p", msg);
	smsType=getSMSType(msg->TargetNumber);
	syslog(LOG_ERR,"sms type: %d",smsType);
	switch (smsType) {
		case SMS_BBS_TYPE_COMMAND:
			retCode=processCommandSMS(msg);
			if (retCode!=PARSE_ERROR) {
				break;
			}
		case SMS_BBS_TYPE_COMMON:
			retCode=deliverSMS(msg);
			break;
		default:
			syslog(LOG_ERR," received unknown sms, targetNumber is : %s ", msg->TargetNumber);
			retCode=ERROR;
	}
	if (retCode==SUCCESS) {
		m_SMSLogger.logIt(msg->SenderNumber, msg->TargetNumber,"",0,m_childCode,msg->parentID,msg->sendTime,time(NULL),msg->arriveTime,msg->SMSBody,msg->SMSBodyLength,SMS_TRANSFER_UP);
	}
	return retCode;

}
/* Send()
 * }}} */
	


	~CSMSBBSChildProtocol() {
		delete m_pChildPrivilegeChecker;
		delete m_pSMSFeeCodeGetter;
	}
};

}

#endif
