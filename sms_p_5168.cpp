#include "sms5168protocol.h"
#include "smsdiskstorage.h"

using namespace SMS;
using namespace std;

class CMyDaemon: public CSMSDaemon{
public:
	

	CMyDaemon(char* applicationName,int logFacility): CSMSDaemon(applicationName,logFacility){
		m_pSMSProtocol=new CSMS5168Protocol;
		m_pSMSStorage = new CSMSDiskStorage(m_pSMSProtocol,SMSHOME "inbox/deliver",SMSHOME "outbox/5168", SMSHOME "outbox.back/5168");

	}
};

CMyDaemon myDaemon("sms_p_5168",LOG_LOCAL0);
