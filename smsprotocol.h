#ifndef SMS_H_7E5F71DD_1299_4446_BA4B_747BE09F601E
#define SMS_H_7E5F71DD_1299_4446_BA4B_747BE09F601E

#include "sms.h"

namespace SMS {

class CSMSProtocol
{
	protected:
	public:
		virtual int  Run(CSMSStorage* pSMSStorage)=0;
		virtual int Send(SMSMessage* msg)=0;
	public:
		CSMSProtocol() {};
		virtual ~CSMSProtocol() {};

};

}



#endif
