#include "smsbbschildprotocol.h"
#include "smsdiskstorage.h"

using namespace SMS;
using namespace std;

class CMyDaemon: public CSMSDaemon{
public:
	

	CMyDaemon(char* applicationName,int logFacility): CSMSDaemon(applicationName,logFacility){
		m_pSMSProtocol=new CSMSBBSChildProtocol("16","k core niu man!","166.111.8.238",50016);
		m_pSMSStorage = new CSMSDiskStorage(m_pSMSProtocol,SMSHOME "inbox/bbs_smth",SMSHOME "outbox/deliver");
	}
};

CMyDaemon myDaemon("sms_c_bbs_smth",LOG_LOCAL0);
