
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <fcntl.h>     
#include <signal.h>
#include <stdio.h>
#include <unistd.h>


#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/unistd.h>

#include <dirent.h>
#include <string>
#include <stdexcept>

#include "deliver.h"






namespace SMS{

#define SIGDSNOTIFY (SIGRTMIN+1)

CDeliver _myDaemon(SMSHOME "outbox/deliver","sms_deliver_c2p",LOG_LOCAL0);

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
	return SMSHOME "outbox/18dx";
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
					syslog(LOG_ERR,"can't link %s to %s",oldpath.c_str(),newpath.c_str());
					throw std::runtime_error("link failed!");
				}

				if (unlink(oldpath.c_str())){
					syslog(LOG_ERR,"can't unlink %s ",oldpath.c_str());
					throw std::runtime_error("unlink failed!");
				}
				syslog(LOG_ERR,"deliver msg: %s", pDirInfo->d_name);
			}
		}
	} catch(std::exception e ) {
			syslog(LOG_ERR, "deliver SMS error: %s", e.what());
	}
	if (m_pDIR) closedir(m_pDIR);
	return 0;	
}

}
