#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <unistd.h>

#include <string.h>

#include <stdlib.h>

#include "testmsg.h"

#include <iostream>

using namespace std;

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

	char szBuf[]="ÄãÎ¥·´ÁËµÚÁùÈÕ·¨ÂÉ£¬¿ËÂ¡ÈË½«²»ÏíÊÜÈÎºÎÈ¨Á¦£¬Äã½«ÔÚ24Ð¡Ê±Ö®ÄÚ±»»ÙÃð¡­¡­";
		int lenPack= sizeof(TestMsg)+strlen(szBuf);
			char* buffer=new char[lenPack];
				TestMsg* ps=(TestMsg *)buffer;
					memset(ps,0,lenPack);
						ps->length=lenPack;
							strcpy(ps->SenderNumber,"13601369910");
								strcpy(ps->TargetNumber,"13693337441"); // ¿¸¸
									ps->SMSBodyLength=strlen(szBuf);
										memcpy(ps+1,szBuf,strlen(szBuf));

	       write(s,ps,lenPack);
										
	close(s);

	delete[] buffer;


	return 0;
}
