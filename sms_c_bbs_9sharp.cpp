#include "smsbbschildprotocol.h"
#include "smsdiskstorage.h"

using namespace SMS;
using namespace std;

class CMyDaemon: public CSMSDaemon{
public:
	

	CMyDaemon(char* applicationName,int logFacility): CSMSDaemon(applicationName,logFacility){
		m_pSMSProtocol=new CSMSBBSChildProtocol("12","bbsbad","166.111.136.8",50012,5000);
		m_pSMSStorage = new CSMSDiskStorage(m_pSMSProtocol,SMSHOME "outbox/deliver",SMSHOME "inbox/bbs_9sharp");
	}
};

CMyDaemon myDaemon("sms_c_bbs_9sharp",LOG_LOCAL0);
