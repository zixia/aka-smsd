#include <stdio.h>
#include <string.h>

#include <stdlib.h>

#include "sms.h"

using namespace SMS;

int main(){
	FILE * fp;

	fp=fopen("testmsg", "wb");

	char szBuf[]="Ê¦¸¸£¬³Ô·¹È¥";
	int lenPack= sizeof(TestMsg)+strlen(szBuf);
	char* buffer=new char[lenPack];
	SMSMessage* ps=(SMSMessage *)buffer;
	memset(ps,0,lenPack);
	ps->length=lenPack;
	strcpy(ps->SenderNumber,"13693337441");
	strcpy(ps->TargetNumber,"13693337441"); // Ê¦¸¸
	ps->SMSBodyLength=strlen(szBuf);
	memcpy(ps+1,szBuf,strlen(szBuf));

	delete[] buffer;


	return 0;
}
