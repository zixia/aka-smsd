#ifndef AKA_SMS_1B4EFFF8_B281_4d10_A28A_94328C7EDA92
#define AKA_SMS_1B4EFFF8_B281_4d10_A28A_94328C7EDA92

namespace SMS{

class CSMSFeeCodeGetter{
public:
	virtual int getFee(int feeType, int* pFeeMoney)=0;
};


}

#endif
