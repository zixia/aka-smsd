#ifndef SMS_696DA6EA_CDE6_4215_BA6F_EE8E8B25202B
#define SMS_696DA6EA_CDE6_4215_BA6F_EE8E8B25202B

#include <sqlplus.hh>
#include "sms.h"
#include <unistd.h>
#include <linux/unistd.h>
#include <string.h>
#include <string>

namespace SMS{

class CSMSBBSChildPrivilegeChecker{
	std::string m_childCode,m_password,m_addr;
	Connection *m_pConn;
public:
	CSMSBBSChildPrivilegeChecker(Connection *pConn):m_childCode(childCode),
	m_pConn(pConn){
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
	int canUserConnect(const char* childCode, const char* password){
		if (strcmp(m_childCode.c_str(),childCode)) 
			return FALSE;
		if (strcmp(m_password.c_str(),password)) 
			return FALSE;
		return TRUE;
	}
};



}


#endif
