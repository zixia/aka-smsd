#include "sms.h"

#include "smsdiskstorage.h"
#include "smstestchildprotocol.h"

using namespace SMS;
using namespace std;

class CMyDaemon: public CSMSDaemon{
public:
	

	CMyDaemon(char* applicationName,int logFacility): CSMSDaemon(applicationName,logFacility){
		m_pSMSProtocol=new CSMSTestProtocol;
		m_pSMSStorage = new CSMSDiskStorage(m_pSMSProtocol,"","/home/roy/gateway/goin");
	}
};

CMyDaemon myDaemon("testrec18dx",LOG_LOCAL0);
