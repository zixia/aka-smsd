#ifndef SMS_696DA6EA_CDE6_4215_BA6F_EE8E8B25202B
#define SMS_696DA6EA_CDE6_4215_BA6F_EE8E8B25202B

#include "sms.h"
#include <unistd.h>
#include <linux/unistd.h>
#include <string.h>

#define USER "test"
#define PASSWORD "test123"

namespace SMS{

class CSMSChildPrivilegeChecker{
public:
	int isConnectPermitted(const char * addr, unsigned short int port){
		syslog(LOG_ERR,"check connect permission %s:%d",addr,port);
		return TRUE;
	}
	int isMsgValid(PSMSMessage msg){
		syslog(LOG_ERR,"validate permission");
		return TRUE;
	}
	int canUserConnect(const char* user, const char* password){
		syslog(LOG_ERR,"+%s+ +%s+",user,password);
		if (strcmp(user,USER)) 
			return FALSE;
		if (strcmp(password,PASSWORD)) 
			return FALSE;
		return TRUE;
	}
};

}


#endif
