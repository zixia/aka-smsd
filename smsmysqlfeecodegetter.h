#ifndef AKA_SMS_ACE5771E_097A_41e8_8680_BE313A126EFD
#define AKA_SMS_ACE5771E_097A_41e8_8680_BE313A126EFD

#include <sqlplus.hh>
#include <time.h>
#include <sstream>
#include <string>
#include "sms.h"
#include "smsfeecodegetter.h"

namespace SMS{

class CSMSMysqlFeeCodeGetter:public CSMSFeeCodeGetter{
	Connection *m_pConn;
	int isOutterConn;
	public:
		CSMSMysqlFeeCodeGetter(Connection* pConn=NULL){
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

		int getFee(int feeType, int* pFeeMoney){
			try{
				Query query=m_pConn->query();
				query<< "select * from SMSFeeCode_TB where feeType="<<feeType;
				Result res=query.store();
				if (res.size()!=0) {
					Row row=*(res.begin());
					*pFeeMoney=atoi(row["FeeMoney"]);
					return SUCCESS;
				} 
			} catch ( BadQuery er) {
				syslog(LOG_ERR," mysql query err : %s", er.error.c_str());
			}
			return FAILED;
		}

		
		~CSMSMysqlFeeCodeGetter(){
			if (!isOutterConn) {
				delete m_pConn;
			}
		}

};


}

#endif
