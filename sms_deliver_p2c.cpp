#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <fcntl.h>     
#include <signal.h>
#include <stdio.h>
#include <unistd.h>


#include <sqlplus.hh>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/unistd.h>

#include <dirent.h>
#include <string>
#include <cstring>
#include <stdexcept>
#include "sms.h"
#include "deliver.h"



using namespace std;


namespace SMS{

#define SIGDSNOTIFY (SIGRTMIN+1)

	
static Connection* pConn=NULL;

CDeliver _myDaemon(SMSHOME "inbox/deliver","sms_deliver_p2c",LOG_LOCAL0);

void __smsDiskStorage_notify_handler(int sig, siginfo_t *si, void *data)
{
	 _myDaemon.OnNotify();
}

int CDeliver::OnSignalTerm(){
	syslog(LOG_ERR,"Terminated by SIGTERM(kill)");
	exit(0);
	return 0;
}

int CDeliver::set_notifier(){
	struct sigaction act;
	act.sa_sigaction = __smsDiskStorage_notify_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	sigaction(SIGDSNOTIFY, &act, NULL);
	int fd=open(m_IncomingDirectory.c_str(), O_RDONLY);

	syslog(LOG_ERR, "start monitor: %s", m_IncomingDirectory.c_str());
	fcntl(fd, F_SETSIG, SIGDSNOTIFY );
	fcntl(fd, F_NOTIFY, DN_CREATE|DN_MULTISHOT);

	return 0;
}

std::string CDeliver::getDest(const std::string& filename){
	char *childCode=strstr(filename.c_str(),".")+1;
	syslog(LOG_ERR,"childCode %s",childCode);
	if (pConn==NULL) {
		pConn=new Connection();
		try {
			pConn->connect(DB_NAME, DB_HOST, DB_USER, DB_PASSWORD);
		} catch (BadQuery er) {
			syslog(LOG_ERR," connect DB error: %s",er.error.c_str());
			return SMSHOME "inbox/default";
		}
	}
	try{
		Query query=pConn->query();
		query<< "select * from SMSChildUser_TB where strcmp(childID,left('"<<childCode<<"',length(childID)))=0";
		Result res=query.store();
		if (res.size()!=0) {
			Row row=*(res.begin());
			return (std::string(SMSHOME "inbox/bbs_")+string(row["childName"]));
		} else {
			syslog(LOG_ERR,"child: %s not found", childCode);
		}
	} catch ( BadQuery er) {
		syslog(LOG_ERR," mysql query err : %s", er.error.c_str());
	}
	return SMSHOME "inbox/default";
}

int CDeliver::Run(){
	openlog("deliver",LOG_PID,LOG_LOCAL0);

	set_notifier();

	for(;;){
	  pause();
	}

	return 0;
}

int CDeliver::OnNotify(){
	syslog(LOG_ERR, "there is new msg!");
	DIR * m_pDIR;
	try{
		m_pDIR=opendir(m_IncomingDirectory.c_str());
		if (m_pDIR==NULL)
		{
			std::string errorMsg="open dir ";
			errorMsg+=m_IncomingDirectory;
			errorMsg+="error" ;
			throw std::runtime_error(errorMsg);		
		}
		
		dirent* pDirInfo;
		struct stat statInfo;

		for (;;) {
			pDirInfo=readdir(m_pDIR);
			if (pDirInfo==NULL)
			{
				break;
			}
			syslog(LOG_ERR,"check file:%s",pDirInfo->d_name);
			std::string path=m_IncomingDirectory+"/"+pDirInfo->d_name;
			stat(path.c_str(),&statInfo);
			if (S_ISREG(statInfo.st_mode))
			{
				std::string oldpath,newpath;
				oldpath=m_IncomingDirectory+"/"+pDirInfo->d_name;
				newpath=getDest(pDirInfo->d_name)+"/"+pDirInfo->d_name;
				if (link(oldpath.c_str(),newpath.c_str())){
					syslog(LOG_ERR,"can't link %s to %s :errno %d",oldpath.c_str(),newpath.c_str(),errno);
				}

				if (unlink(oldpath.c_str())){
					syslog(LOG_ERR,"can't unlink %s : errno %d",oldpath.c_str(),errno);
				}
				syslog(LOG_ERR,"deliver msg: %s to %s", oldpath.c_str(),newpath.c_str());
			}
		}
	} catch(std::exception e ) {
			syslog(LOG_ERR, "deliver SMS error: %s", e.what());
	}
	if (m_pDIR) closedir(m_pDIR);
	return 0;	
}

}
