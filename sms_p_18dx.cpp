#include "sms18dxprotocol.h"
#include "smsdiskstorage.h"

using namespace SMS;
using namespace std;

class CMyDaemon: public CSMSDaemon{
public:
	

	CMyDaemon(char* applicationName,int logFacility): CSMSDaemon(applicationName,logFacility){
		m_pSMSProtocol=new CSMS18DXProtocol;
		m_pSMSStorage = new CSMSDiskStorage(m_pSMSProtocol,SMSHOME "inbox/deliver",SMSHOME "outbox/18dx");

	}
};

CMyDaemon myDaemon("sms_p_18dx",LOG_LOCAL0);
