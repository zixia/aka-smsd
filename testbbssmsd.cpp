#include "smsbbschildprotocol.h"
#include "smsdiskstorage.h"

using namespace SMS;
using namespace std;

class CMyDaemon: public CSMSDaemon{
public:
	

	CMyDaemon(char* applicationName,int logFacility): CSMSDaemon(applicationName,logFacility){
		m_pSMSProtocol=new CSMSBBSChildProtocol("12","bbsbad","",4002);
		m_pSMSStorage = new CSMSDiskStorage(m_pSMSProtocol,"/home/roy/gateway/in","/home/roy/gateway/bbsgo");
	}
};

CMyDaemon myDaemon("testbbschild",LOG_LOCAL0);
