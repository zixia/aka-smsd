#ifndef SMS_H_4349655C_9F52_4cb3_B691_8B40892AC56D
#define SMS_H_4349655C_9F52_4cb3_B691_8B40892AC56D
#include "app.h"

namespace SMS {

class CDeliver: public RCL::CApplication{
	std::string m_IncomingDirectory;
protected:
	std::string getDest(const std::string& filename) ;
	int set_notifier();
public:
	CDeliver(const std::string & incomingDirectory,char* applicationName,int logFacility):m_IncomingDirectory(incomingDirectory){
		openlog(applicationName,LOG_PID,logFacility);		
	};
	
	int IsDaemon(){
		return 1;
	}
	
	int Run();

	int OnSignalTerm();

	int OnNotify();
};

}


#endif
