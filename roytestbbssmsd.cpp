#include "smsbbschildprotocol.h"
#include "smsdiskstorage.h"

using namespace SMS;
using namespace std;

class CMyDaemon: public CSMSDaemon{
public:
	

	CMyDaemon(char* applicationName,int logFacility): CSMSDaemon(applicationName,logFacility){
		m_pSMSProtocol=new CSMSBBSChildProtocol("15","roytest","210.82.111.67",4005);
		m_pSMSStorage = new CSMSDiskStorage(m_pSMSProtocol,"/home/roy/gateway/in","/home/roy/gateway/roytestbbsgo");
	}
};

CMyDaemon myDaemon("roytestbbschild",LOG_LOCAL0);
