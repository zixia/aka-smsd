#include "smsbbschildprotocol.h"
#include "smsdiskstorage.h"

using namespace SMS;
using namespace std;

class CMyDaemon: public CSMSDaemon{
public:
	

	CMyDaemon(char* applicationName,int logFacility): CSMSDaemon(applicationName,logFacility){
		m_pSMSProtocol=new CSMSBBSChildProtocol("13","zixiabbs","202.205.10.10",50013);
		m_pSMSStorage = new CSMSDiskStorage(m_pSMSProtocol,SMSHOME "inbox/bbs_zixia",SMSHOME "outbox/deliver");
	}
};

CMyDaemon myDaemon("sms_c_bbs_zixia",LOG_LOCAL0);
