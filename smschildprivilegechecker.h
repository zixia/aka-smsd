#ifndef SMS_696DA6EA_CDE6_4215_BA6F_EE8E8B25202B
#define SMS_696DA6EA_CDE6_4215_BA6F_EE8E8B25202B

#include "sms.h"
#include <unistd.h>
#include <linux/unistd.h>
#include <string>

namespace SMS{

class CSMSChildPrivilegeChecker{
public:
	int isConnectPermitted(const char * addr, unsigned short int port){
		syslog(LOG_ERR,"check connect permission");
		return TRUE;
	}
	int isMsgValid(PSMSMessage msg){
		syslog(LOG_ERR,"validate permission");
		return TRUE;
	}
};

}


#endif
