#include "smsbbschildprotocol.h"
#include "smsdiskstorage.h"

using namespace SMS;
using namespace std;

class CMyDaemon: public CSMSDaemon{
public:
	

	CMyDaemon(char* applicationName,int logFacility): CSMSDaemon(applicationName,logFacility){
		m_pSMSProtocol=new CSMSBBSChildProtocol(50230,5000);
		m_pSMSStorage =NULL;
	}
};

CMyDaemon myDaemon("sms_c_bbs                  ",LOG_LOCAL0);
