#include "smschildprotocol.h"
#include "smsdiskstorage.h"

#include "sms.h"

using namespace SMS;
using namespace std;

class CMyDaemon: public CSMSDaemon{
public:
	

	CMyDaemon(char* applicationName,int logFacility): CSMSDaemon(applicationName,logFacility){
		m_pSMSProtocol=new CSMSChildProtocol;
		m_pSMSStorage = new CSMSDiskStorage(m_pSMSProtocol,SMSHOME "outbox/deliver", SMSHOME "inbox/default", SMSHOME "inbox.back/default");
	}
};

CMyDaemon myDaemon("testchild",LOG_LOCAL0);
