#include "sms.h"


using namespace SMS;
using namespace std;

int CSMSStorage::OnNotify(){
	syslog(LOG_ERR, "there is new msg!");
	try{
		if (getFirstSMSFromStorage()!=0) {
			throw runtime_error("Storage is empty");
		}
		char* buf=NULL;
		unsigned int bufLen=0;
		unsigned int dataLen=0;
		do{
			dataLen=bufLen;
			readGettedSMS(buf,&dataLen);
			if (dataLen>bufLen)
			{
				delete[] buf;
				buf=new char[dataLen];
				bufLen=dataLen;
				readGettedSMS(buf,&dataLen);
				if (dataLen>bufLen)
				{
					syslog(LOG_ERR,"read SMS length error!");
					throw runtime_error("read SMS length error!");
				}
			}
			m_pSMSPProtocol->Send((SMSMessage *)buf);  //todo
		}
		while (getNextSMSFromStorage()==0);
		delete[] buf;
	} catch(exception e ) {
			syslog(LOG_ERR, "Send SMS error: %s", e.what());
	}
	clearStorage();
	return 0;
}

