#ifndef AKA_SMS_3BBDE74D_F9EF_401C_9C6D_FC3327ED0A0D
#define AKA_SMS_3BBDE74D_F9EF_401C_9C6D_FC3327ED0A0D

#include <sqlplus.hh>
#include <time.h>
#include <sstream>
#include <string>
#include "sms.h"

namespace SMS{

typedef enum _SMS_CONTENT_TYPE{
SMS_CONTENT_TYPE_TXT, SMS_CONTENT_TYPE_BIN
} SMS_CONTENT_TYPE;

class CSMSLogger{
	Connection *m_pConn;
	int isOutterConn;
	void convertDate(const time_t& t, char* buf){
		struct tm tml;
		gmtime_r(&t, &tml);
		sprintf(buf,"%04d-%02d-%02d %02d:%02d:%02d",tml.tm_year+1900,tml.tm_mon+1,tml.tm_mday,tml.tm_hour,tml.tm_min,tml.tm_sec);
	}
	public:
		CSMSLogger(Connection* pConn=NULL){
			if (pConn!=NULL) {
				m_pConn=pConn;
				isOutterConn=TRUE;
			} else {
				m_pConn=new Connection(use_exceptions);
				try {
					m_pConn->connect(DB_NAME, DB_HOST, DB_USER, DB_PASSWORD);
				} catch (BadQuery er) {
					syslog(LOG_ERR," connect DB error: %s",er.error.c_str());
					exit(-1);
				}
			}
		}

		int logIt(const char* sourceNo, const char* targetNo, const char* feeTargetNo, int feeTypeID, const char * childID, const char* parentID, const time_t &  sendTime,
				const time_t& deliverTime, const time_t& arriveTime, const char* content, const SMS_CONTENT_TYPE& contentType=SMS_CONTENT_TYPE_TXT) {
			try{
				std::stringstream sql;
				char strDeliverTime[25];
				char strSendTime[25];
				char strArriveTime[25];
				
				convertDate(sendTime,strSendTime);
				convertDate(deliverTime,strDeliverTime);
				//convertDate(arriveTime,strArriveTime);


				sql<<"insert into SMSLog_TB(sourceNo, targetNo,feeTargetNo,feeType,childID, parentID, sentTime, deliveTime, content ) values ('"
					<<sourceNo<<"','"<<targetNo<<"','"<<feeTargetNo<<"',"<<feeTypeID<<",'"<<childID<<"','"<<parentID<<"','"
					<<strSendTime<<"','"<<strDeliverTime<<"','"<<content<<"')";
				m_pConn->exec(sql.str());

				} catch ( BadQuery er) {
					syslog(LOG_ERR,"log sms error -- mysql query err : %s", er.error.c_str());
					return FAILED;
				}
			return SUCCESS;
		}
		
		~CSMSLogger(){
			if (!isOutterConn) {
				delete m_pConn;
			}
		}

};


}

#endif
