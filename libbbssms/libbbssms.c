#include "libbbssms.h"
#include "protocol.h"
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <unistd.h>

#include <string.h>

#include <stdlib.h>


#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

static int sockfd;

static DWORD sms_byteToLong(byte arg[4]) {
	unsigned long int tmp;
	tmp=(arg[0]<<24)+(arg[1]<<16)+(arg[2]<<8)+arg[3];
	return tmp;
}

static void sms_longToByte(byte* arg, DWORD num) {
	arg[0]=num>>24;
	arg[1]=(num>>16) & 0xff;
	arg[2]=(num>>8) & 0xff;
	arg[3]=num & 0xff;
}
static int sms_read(int sockfd, void* buf, size_t size) {
	int rc;
	rc=read(sockfd, buf, size);
	if (rc<0) {
		return rc;
	}
	if (rc==0) {
		close(sockfd);
		sockfd=-1;
		return SMS_ERR_GWCLOSED;
	}
}

int sms_init(){
	sockfd=-1;
}

//建立socket连接后登录网关
int sms_login(DWORD serialNo, char* gatewayAddress, unsigned int gatewayPort, char* user, char* pass){
	SMS_BBS_LOGINPACKET packet;
	struct hostent * phe;
	struct protoent *ppe;
	struct sockaddr_in sin;
	int rc;
	int type;
	SMS_BBS_HEADER head;

	memset(&sin,0,sizeof(sin));
	sin.sin_family=AF_INET;

	sin.sin_port=htons(gatewayPort);

	if (phe = gethostbyname(gatewayAddress) )	{
		memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
	} else if (( sin.sin_addr.s_addr = inet_addr (gatewayAddress)) == INADDR_NONE)
	{
		return SMS_ERR_DNS;
	}
	if ( ( ppe=getprotobyname("tcp")) == 0)
	{
		return SMS_ERR_TCPPROTOCOL;
	}
	type=SOCK_STREAM;
	sockfd=socket(PF_INET, type,ppe->p_proto);
	if (sockfd<0)
	{
		return SMS_ERR_SOCKET;
	}
	if (connect(sockfd,(struct sockaddr * )&sin,sizeof(sin))<0)
	{
		return SMS_ERR_CONNECT;
	}
	packet.header.Type=SMS_BBS_CMD_LOGIN;
	sms_longToByte(packet.header.SerialNo,serialNo);
	sms_longToByte(packet.header.pid,0);
	sms_longToByte(packet.header.msgLength, sizeof(packet)-sizeof(SMS_BBS_HEADER));

	strncpy(packet.user, user,SMS_USER_LEN);
	packet.user[SMS_USER_LEN]=0;
	strncpy(packet.password, pass,SMS_PASS_LEN);
	packet.password[SMS_PASS_LEN]=0;
	if (( rc=write(sockfd, &packet, sizeof(packet))) <0) 
		return rc;
	
	rc = sms_read(sockfd, &head, sizeof(head));
	if (rc<0) {
		return rc;
	}

	if (head.Type==SMS_BBS_CMD_OK) {
		return 0;
	} else {
		return SMS_ERR_LOGIN;
	}	
}

int sms_sendRegisterNumber(DWORD  serialNo, int pid, char* mobilePhoneNumber, char* userID){
	SMS_BBS_REGISTERMOBILEPACKET packet;
	if (sockfd<0) {
		return	SMS_ERR_NOT_CONNECTED;
	}	
	packet.header.Type=SMS_BBS_CMD_REG;
	sms_longToByte(packet.header.SerialNo,serialNo);
	sms_longToByte(packet.header.pid,pid);
	sms_longToByte(packet.header.msgLength, sizeof(packet)-sizeof(SMS_BBS_HEADER));

	strncpy(packet.MobileNo, mobilePhoneNumber, MOBILENUMBERLENGTH);
	packet.MobileNo[MOBILENUMBERLENGTH]=0;
	strncpy(packet.cUserID, userID, SMS_BBS_ID_LEN);
	packet.cUserID[SMS_BBS_ID_LEN]=0;
	return write(sockfd, &packet, sizeof(packet));
}

//请求网关检查输入的绑定注册码是否正确
int sms_checkRegisterNumber(DWORD serialNo, int pid, char* mobilePhoneNumber, char* userID, char* Code){
	SMS_BBS_REGISTERVALIDATIONPACKET packet;
	if (sockfd<0) {
		return	SMS_ERR_NOT_CONNECTED;
	}	
	packet.header.Type=SMS_BBS_CMD_CHECK;
	sms_longToByte(packet.header.SerialNo,serialNo);
	sms_longToByte(packet.header.pid,pid);
	sms_longToByte(packet.header.msgLength, sizeof(packet)-sizeof(SMS_BBS_HEADER));

	strncpy(packet.MobileNo, mobilePhoneNumber, MOBILENUMBERLENGTH);
	packet.MobileNo[MOBILENUMBERLENGTH]=0;
	strncpy(packet.cUserID, userID, SMS_BBS_ID_LEN);
	packet.cUserID[SMS_BBS_ID_LEN]=0;
	strncpy(packet.ValidateNo, Code, SMS_VALID_LEN);
	packet.ValidateNo[SMS_VALID_LEN]=0;
	return write(sockfd, &packet, sizeof(packet));
}

//请求网关取消手机绑定
int sms_unRegister(DWORD serialNo, int pid, char* mobilePhoneNumber, char* userID){
	SMS_BBS_UNREGISTERMOBILEPACKET packet;
	if (sockfd<0) {
		return	SMS_ERR_NOT_CONNECTED;
	}	
	packet.header.Type=SMS_BBS_CMD_UNREG;
	sms_longToByte(packet.header.SerialNo,serialNo);
	sms_longToByte(packet.header.pid,pid);
	sms_longToByte(packet.header.msgLength, sizeof(packet)-sizeof(SMS_BBS_HEADER));

	strncpy(packet.MobileNo, mobilePhoneNumber, MOBILENUMBERLENGTH);
	packet.MobileNo[MOBILENUMBERLENGTH]=0;
	strncpy(packet.cUserID, userID, SMS_BBS_ID_LEN);
	packet.cUserID[SMS_BBS_ID_LEN]=0;
	return write(sockfd, &packet, sizeof(packet));
}

//请求网关发送手机短信
int sms_sendSMS(DWORD serialNo, int pid,DWORD userBBSCode,  char *userID, char * sourcePhoneNumber, char* targetPhoneNumber, char* content, int contentLen, int contentType){
	SMS_BBS_BBSSENDSMS packet;
	int rc;
	if (sockfd<0) {
		return	SMS_ERR_NOT_CONNECTED;
	}	
	packet.header.Type=SMS_BBS_CMD_BBSSEND;

	if (contentType!=SMS_MSG_TYPE_TXT) {
		return SMS_ERR_MSGTYPE;
	}

	sms_longToByte(packet.header.SerialNo,serialNo);
	sms_longToByte(packet.header.pid,pid);
	sms_longToByte(packet.header.msgLength, sizeof(packet)-sizeof(SMS_BBS_HEADER)+contentLen);

	sms_longToByte(packet.UserID, userBBSCode);

	strncpy(packet.SrcMobileNo, sourcePhoneNumber, MOBILENUMBERLENGTH);
	packet.SrcMobileNo[MOBILENUMBERLENGTH]=0;
	strncpy(packet.srcUserID, userID, SMS_BBS_ID_LEN);
	packet.srcUserID[SMS_BBS_ID_LEN]=0;
	strncpy(packet.DstMobileNo, targetPhoneNumber, MOBILENUMBERLENGTH);
	packet.DstMobileNo[MOBILENUMBERLENGTH]=0;

	sms_longToByte(packet.MsgTxtLen, contentLen);
#ifdef NDEBUG
	{
		char buf[500];
		int i;
		for (i=0;i<sizeof(packet.header);i++)
			printf("%02X ",*(((char*)&packet.header)+i));
		buf[i]=0;
		printf("\n",buf);
	}
#endif

	rc=write(sockfd, &packet, sizeof(packet));
	if (rc<0) {
		return rc;
	}
	return write(sockfd,content,contentLen);
}

//回应网关的手机绑定请求
int sms_replyGWRegister(DWORD serialNo, int pid ,int isSucceed){
	SMS_BBS_BINDREQUESTREPLYPACKET packet;
	if (sockfd<0) {
		return	SMS_ERR_NOT_CONNECTED;
	}	
	packet.header.Type=SMS_BBS_CMD_REQUESTREPLY;

	sms_longToByte(packet.header.SerialNo,serialNo);
	sms_longToByte(packet.header.pid,pid);
	sms_longToByte(packet.header.msgLength, sizeof(packet)-sizeof(SMS_BBS_HEADER));

	packet.isSucceed=isSucceed?1:0;

	packet.MobileNo[0]=0;

	return write(sockfd, &packet, sizeof(packet));
}

//从网关读取操作回复、短信等信息
int sms_getReply(DWORD* pSerialNo, int *pPid, int *pReplyType, void** pBuf){
	SMS_BBS_HEADER head;
	int len;
	int rc;
	if (sockfd<0) {
		return	SMS_ERR_NOT_CONNECTED;
	}	

	rc = sms_read(sockfd, &head, sizeof(head));
	if (rc<0) {
		return rc;
	}

	*pSerialNo=sms_byteToLong(head.SerialNo);
	*pPid=sms_byteToLong(head.pid);

	len=sms_byteToLong(head.msgLength);
	switch(head.Type) {

		case SMS_BBS_CMD_REQUEST: {
						  PSMS_BINDREQUEST p;
						  SMS_BBS_BINDREQUESTPACKET pack;
						  p=(PSMS_BINDREQUEST)malloc(sizeof(SMS_BINDREQUEST));
						  if (p==NULL) {
							  return SMS_ERR_MEMORY;
						  }
						  *pBuf=p;
						  rc=read(sockfd, &pack, len);
						  if (rc<0) {
							  return rc;
						  }
						  *pReplyType=pack.Bind==1 ? SMS_CMD_REGISTER : SMS_CMD_UNREGISTER;
						  strncpy(p->userID, pack.cUserID, SMS_BBS_ID_LEN);
						  p->userID[SMS_BBS_ID_LEN]=0;
						  strncpy(p->MobileNo, pack.MobileNo, MOBILENUMBERLENGTH);
						  p->MobileNo[MOBILENUMBERLENGTH]=0;
						  return 0;
					  }
		case SMS_BBS_CMD_GWSEND: {
						 PSMS_GWSMS p;
						 PSMS_BBS_GWSENDSMS pack;
						 DWORD msgLen;

						 pack=(PSMS_BBS_GWSENDSMS)malloc(len);
						 if (pack==NULL){
							 return SMS_ERR_MEMORY;
						}

						 rc=read(sockfd, pack, len);
						 if (rc<0) {
							 free(pack);
							 return rc;
						 }
						 msgLen=sms_byteToLong(pack->MsgTxtLen);
						 p=(PSMS_GWSMS)malloc(sizeof(SMS_GWSMS)+msgLen+1);
						 if (p==NULL) {
							 free(pack);
							 return SMS_ERR_MEMORY;
						 }
						 p->MsgTxtLen=msgLen;
						 memcpy(p->MsgTxt,pack->MsgTxt,msgLen);
						 p->MsgTxt[msgLen]=0;
						 if (rc<0) {
							 free(pack);
							 return rc;
						 }
						 
 						 *pBuf=p;
						 *pReplyType=SMS_CMD_GWSMS;

						 p->userBBSCode=sms_byteToLong(pack->UserID);
						 strncpy(p->SrcMobileNo, pack->SrcMobileNo, MOBILENUMBERLENGTH);
						 p->SrcMobileNo[MOBILENUMBERLENGTH]=0;
						 p->MsgType=SMS_MSG_TYPE_TXT;
						 free(pack);
						 return 0;
					 }
		default:
					 *pReplyType=head.Type;
					 return 0;
	}
}

int sms_exit() {
	if (sockfd>0) {
		close(sockfd);
	}
	sockfd=-1;
	return 0;
}

int sms_getsocket(){
	return sockfd;
}

