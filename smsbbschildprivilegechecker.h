#ifndef SMS_696DA6EA_CDE6_4215_BA6F_EE8E8B25202B
#define SMS_696DA6EA_CDE6_4215_BA6F_EE8E8B25202B

#include <sqlplus.hh>
#include "sms.h"
#include <unistd.h>
#include <linux/unistd.h>
#include <string.h>
#include <string>
#include <netinet/in.h>
#include <arpa/inet.h>

       int inet_aton(const char *cp, struct in_addr *inp);


namespace SMS{

class CSMSBBSChildPrivilegeChecker{
	std::string m_childCode;
	Connection *m_pConn;
	int addressInNet(unsigned long int addr, const char* net) {
		char s[50];
		unsigned long int maskLen;
		in_addr in;
		s[0]=0;
		maskLen=0;
		sscanf(net,"%s/%d",s,maskLen);
		if (!inet_aton(s,&in)) 
			return -1;
		}
		if ((addr>>(32-maskLen))!=(in.s_addr>>(32-maskLen)) ) {
			return -1;
		}
		return 0;
	}	
public:
	CSMSBBSChildPrivilegeChecker(Connection *pConn):m_pConn(pConn){
	}
	int isConnectPermitted(const char * addr, unsigned short int port){
		return TRUE;
	}
	int canSendSMS(const char* srcMobileNo, const char* srcID){
		try{
			Query query=m_pConn->query();
			query<< "select * from SMSRegister_TB where childCode='"<<m_childCode<<"' and MobilePhoneNumber='"<<srcMobileNo<<"' and UPPER(srcID)=UPPER('"<<srcID<<"') and ValidatationNumber=''";
			Result res=query.store();
			if (res.size()!=0) {
				return TRUE;
			} else {
				syslog(LOG_ERR,"%s %s not registerd", srcID, srcMobileNo);
				return FALSE;
			}
		} catch ( BadQuery er) {
			syslog(LOG_ERR," mysql query err : %s", er.error.c_str());
			return FALSE;
		}
	}
	int canReceive(const char* targetMobileNo){
		syslog(LOG_ERR,"validate receive permission");
		return TRUE;
	}
	int loginUser(unsigned long int address, const char* childCode, const char* password, char* targetChildCode, char* targetChildName, int* pMoneyLimit){
		try{
			Query query=m_pConn->query();
			query<< "select * from SMSChildUser_TB where childID='"<<childCode<<"' and childPass='"<<password<<"'";
			Result res=query.store();
			if (res.size()!=0) {
				Row row=*(res.begin());
				if (addressInNet(address,row["loginNet"])==0) {
					m_childCode=childCode;
					strncpy(targetChildCode,childCode, SMS_MAXCHILDCODE_LEN);
					targetChildCode[SMS_MAXCHILDCODE_LEN]=0;
					strncpy(targetChildName,row["childName"], SMS_MAXCHILDName_LEN);
					targetChildCode[SMS_MAXCHILDNAME_LEN]=0;
					*pMoneyLimit=atoi(row["defaultMoneyLimit"]);
					return TRUE;
				}
			} else {
				syslog(LOG_ERR,"%s %s not registerd", srcID, srcMobileNo);
			}
		} catch ( BadQuery er) {
			syslog(LOG_ERR," mysql query err : %s", er.error.c_str());
		}
		return FALSE;
	}
};



}


#endif
