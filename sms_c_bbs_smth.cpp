#include "smsbbschildprotocol.h"
#include "smsdiskstorage.h"

using namespace SMS;
using namespace std;

class CMyDaemon: public CSMSDaemon{
public:
	

	CMyDaemon(char* applicationName,int logFacility): CSMSDaemon(applicationName,logFacility){
		m_pSMSProtocol=new CSMSBBSChildProtocol("16","k core niu man!","202.112.58.200",50016);
		m_pSMSStorage = new CSMSDiskStorage(m_pSMSProtocol,SMSHOME "outbox/deliver",SMSHOME "inbox/bbs_smth");
	}
};

CMyDaemon myDaemon("sms_c_bbs_smth",LOG_LOCAL0);
