#include "sms.h"
#include "mgrep.h"


using namespace SMS;
using namespace std;
int CSMSStorage::writeSMStoStorage(const char* sourceNo, const char* TargetNo, char* buf, unsigned int buf_size){
	initFilter();
	if (mgrep_str(buf, buf_size,m_filterBuf)==0) {
		return SUCCESS;
	} 
	syslog(LOG_ERR," msg have bad word!");
	return ERROR;

}

int CSMSStorage::OnNotify(){
	syslog(LOG_ERR, "there is new msg!");
	try{
		if (getFirstSMSFromStorage()!=0) {
			throw runtime_error("Storage is empty");
		}
		char* buf=NULL;
		unsigned int bufLen=0;
		unsigned int dataLen=0;
		int retCode;
		do{
			dataLen=bufLen;
			syslog(LOG_ERR, "shit1!");
			readGettedSMS(buf,&dataLen);
			syslog(LOG_ERR, "shit2!");
			syslog(LOG_ERR,"buf point: %p dataLen: %d", buf, dataLen);
			if (dataLen>bufLen)
			{
				buf=(char*)realloc(buf, dataLen);
			syslog(LOG_ERR,"buf point: %p dataLen: %d", buf, dataLen);
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
			syslog(LOG_ERR,"buf point: %p", buf);
			retCode=m_pSMSPProtocol->Send((SMSMessage *)buf);
			if (retCode!=SUCCESS) {
				if (retCode==ERROR) {
					backupError();
					syslog(LOG_ERR,"send error:(");
				} else {
					syslog(LOG_ERR,"send failed");
				}
			} else {
				recordSended();
			}
			syslog(LOG_ERR, "shit6!");
		}
		while (getNextSMSFromStorage()==0);
		free(buf);
	} catch(exception e ) {
			syslog(LOG_ERR, "Send SMS error: %s", e.what());
	}
	clearStorage();
	set_notifier();
	return 0;
}

