#ifndef MOBILENUMBERLENGTH
#define MOBILENUMBERLENGTH 16  //ºÅÂë³¤¶È
#endif
struct TestMsg{
	unsigned int length;
	char SenderNumber[MOBILENUMBERLENGTH];
	char TargetNumber[MOBILENUMBERLENGTH];
	unsigned int SMSBodyLength;
	char SMSBody[0];
};

const char* testport="6002";
