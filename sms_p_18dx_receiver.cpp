#include "sms18dxrcvprotocol.h"
#include "smsdiskstorage.h"

#include "sms.h"

using namespace SMS;
using namespace std;

class CMyDaemon: public CSMSDaemon{
public:
	

	CMyDaemon(char* applicationName,int logFacility): CSMSDaemon(applicationName,logFacility){
		m_pSMSProtocol=new CSMS18dxRcvProtocol;
		m_pSMSStorage = new CSMSDiskStorage(m_pSMSProtocol,SMSHOME "inbox/deliver","");
	}
};

CMyDaemon myDaemon("sms_p_18dx_receiver",LOG_LOCAL0);
