#ifndef RCL_H_APP_4C9557A02420417f850745DF202C81A5
#define RCL_H_APP_4C9557A02420417f850745DF202C81A5

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <syslog.h>
#include <cassert>
#include <string>
#include <stdexcept>


namespace RCL {

#define MAXFD 64
#define MAKEDAEMONAPP 

class EDaemon:public std::runtime_error
{
	public:
		EDaemon(const std::string & whatString): std::runtime_error(whatString){
	}

};

class CApplication{
public:
	int m_nArgc;
	char** m_strArgv;
	char**  m_pEnv;
	void	InitDaemon();
public:
	int	InitSignal();

	CApplication();
	virtual ~CApplication();

	virtual int IsDaemon(){
		return 0;
	};		/* Return 1 for Daemon, Return 0 for console */



	virtual int Run(){return 1;};

	virtual int OnSignalChild(){return 1;};
	virtual int OnSignalHup(){return 1;};
	virtual int OnSignalInt(){return 1;};
	virtual int OnSignalTerm(){return 1;};
	virtual int OnSignalPipe(){return 1;};
	virtual int OnSignalSegv(){return 1;};
	virtual int OnSignalUser1(){return 1;};
	virtual int OnSignalAlarm(){return 1;};
};

extern CApplication* RCL_GetApp();

}


#endif
