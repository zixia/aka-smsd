#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <unistd.h>

#include <string.h>

#include <stdlib.h>
#include <stdio.h>


#include "../sms.h"
#include "../childprotocol.h"

#include <iostream>


using namespace std;
using namespace SMS;

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

const char host[]="localhost";

int main(){

	hostent * phe;
	servent * pse;
	protoent *ppe;
	sockaddr_in sin;
	int s, type;
	fd_set readset;
	int rv,rc;
	char buf[500];
	int lala=0;

	memset(&sin,0,sizeof(sin));
	sin.sin_family=AF_INET;


	if (pse=getservbyname(testport,"tcp")){
		sin.sin_port=pse->s_port;
	} else if ( (sin.sin_port=htons((unsigned short)atoi(testport) ))==0)	{
		cout<<"get port error."<<endl;
		return -1;
	}

	if (phe = gethostbyname(host) )	{
		memcpy(&sin.sin_addr,phe->h_addr, phe->h_length);
	} else if (( sin.sin_addr.s_addr = inet_addr (host)) == INADDR_NONE)
	{
		cout<<"get host error."<<endl;
		return -1;
	}
	if ( ( ppe=getprotobyname("tcp")) == 0)
	{
		cout<<"get tcp error."<<endl;
		return -1;
	}
	type=SOCK_STREAM;
	s=socket(PF_INET, type,ppe->p_proto);
	cout<<ppe->p_proto;
	exit(-1);
	if (s<0)
	{
		cout<<"get socket error"<<endl;
		return -1;
	}
	if (connect(s,(struct sockaddr * )&sin,sizeof(sin))<0)
	{
		cout<<"connect failed."<<endl;
		return -1;
	}
	for (;;) {
		FD_ZERO(&readset);
		FD_SET(0,&readset);
		FD_SET(s,&readset);
		rv=select(s+1,&readset,NULL,NULL,NULL);
		if (rv==-1){
			break;
		}
		if(FD_ISSET(0, &readset)){
			lala++;
			rc=read(0,buf,sizeof(buf));
			int lenPack= sizeof(SMSChildProtocolSendMessage)+rc;
			char* buffer=new char[lenPack];
			PSMSChildProtocolSendMessage ps=(PSMSChildProtocolSendMessage)buffer;
			memset(ps,0,lenPack);
			ps->head.msgTypeID=MSGTYPE_SM;
			sms_longToByte(ps->head.SMSSerialNo,lala);
			sms_longToByte(ps->head.msgLength,sizeof(SMSChildProtocolSendMessage)-sizeof(SMSChildProtocolHead));
			strcpy(ps->senderNo,"13601369910");
			strcpy(ps->targetNo,"13693337441"); // 扛