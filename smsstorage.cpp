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
			syslog(LOG_ERR, "shit1!");
			readGettedSMS(buf,&dataLen);
			syslog(LOG_ERR, "shit2!");
			if (dataLen>bufLen)
			{
				delete[] buf;
				buf=new char[dataLen];
				if (buf==NULL){
					syslog(LOG_ERR, "alloc memory for sms send error!");
					return -1;
				}
				bufLen=dataLen;
				syslog(LOG_ERR, "shit3!");
				readGettedSMS(buf,&dataLen);
				syslog(LOG_ERR, "shit4!");
				if (dataLen>bufLen)
				{
					syslog(LOG_ERR,"read SMS length error!");
					throw runtime_error("read SMS length error!");
				}
			}
			syslog(LOG_ERR, "shit5!");
			if (m_pSMSPProtocol->Send((SMSMessage *)buf)!=0) {
				syslog(LOG_ERR,"send error:(");
			} else {
				recordSended();
			}
			syslog(LOG_ERR, "shit6!");
		}
		while (getNextSMSFromStorage()==0);
		delete[] buf;
	} catch(exception e ) {
			syslog(LOG_ERR, "Send SMS error: %s", e.what());
	}
	clearStorage();
	return 0;
}

