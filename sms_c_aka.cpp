#include "smschildprotocol.h"
#include "smsdiskstorage.h"

#include "sms.h"

using namespace SMS;
using namespace std;

class CMyDaemon: public CSMSDaemon{
public:
	

	CMyDaemon(char* applicationName,int logFacility): CSMSDaemon(applicationName,logFacility){
		m_pSMSProtocol=new CSMSChildProtocol;
		m_pSMSStorage = new CSMSDiskStorage(m_pSMSProtocol,SMSHOME "outbox/deliver", SMSHOME "inbox/c_aka", SMSHOME "inbox.back/c_aka");
	}
};

CMyDaemon myDaemon("sms_c_aka",LOG_LOCAL0);
