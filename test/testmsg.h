#ifndef MOBILENUMBERLENGTH
#define MOBILENUMBERLENGTH 16  //号码长度
#endif
struct TestMsg{
	unsigned int length;
	char SenderNumber[MOBILENUMBERLENGTH];
	char TargetNumber[MOBILENUMBERLENGTH];
	unsigned int SMSBodyLength;
	char SMSBody[0];
};

const char* testport="6002";
