#ifndef SMS_A00B8199_324E_45A7_AEB4_54FDB4784FE6
#define SMS_A00B8199_324E_45A7_AEB4_54FDB4784FE6

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <linux/unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string>
#include <cassert>
#include <ctime>
#include <fstream>
#include <vector>
#include <string.h>
#include <sstream>
#include "sms.h"


namespace SMS{

#define BACKUPDIR SMSHOME "error"

class CSMSDiskStorage: public CSMSStorage {
	std::string m_OutgoingDirectory;
	std::string m_IncomingDirectory;
	std::string m_currFilename;
	DIR * m_pDIR;
	std::vector<std::string> m_readedFileList;
	char* m_pDataBuf;
	unsigned int m_dataSize;
	unsigned int m_bufSize;
	int m_fp;


public:
	CSMSDiskStorage(CSMSProtocol *pSMSPProtocol, const std::string & OutgoingDirectory, const std::string & IncomingDirectory );

	~CSMSDiskStorage();

	int set_notifier();

	int writeSMStoStorage(const char* sourceNo, const char* TargetNo, char* buf, unsigned int buf_size){
		int errCode;
		std::time_t lt;
		std::ofstream os;
		lt=time(NULL);
		std::stringstream filename;
		filename<<m_OutgoingDirectory<<"/"<<sourceNo<<"."<<TargetNo<<"."<<lt<<'\0';
		os.exceptions(std::ios::badbit|std::ios::failbit|std::ios::eofbit);
		syslog(LOG_ERR, "write new msg to %s", filename.str().c_str());
		try{
			os.open(filename.str().c_str());
			os.write(buf,buf_size);
			/*	
			os<<"from:"<<((SMSMessage*)(buf))->SenderNumber<<std::endl;
			os<<"to:"<<((SMSMessage*)(buf))->TargetNumber<<std::endl;
			os<<"BodyLength:"<<((SMSMessage*)(buf))->SMSBodyLength<<std::endl;
			os<<"Body:";
			os.write(((SMSMessage*)(buf))->SMSBody, ((SMSMessage*)(buf))->SMSBodyLength);
			*/
			os.close();
		} catch (std::exception e){
			syslog(LOG_ERR, "write file %s error : %s", filename.str().c_str(), e.what());
			return ERROR;
		}
		return SUCCESS;
	};
	
	int getNextSMSFromStorage() {

		dirent* pDirInfo;
		struct stat statInfo;
		for (;;) {
			pDirInfo=readdir(m_pDIR);
			if (pDirInfo==NULL)
			{
				return -1;
			}
			syslog(LOG_ERR,"check file:%s",pDirInfo->d_name);
			std::string path=m_IncomingDirectory+"/"+pDirInfo->d_name;
			stat(path.c_str(),&statInfo);
			if (S_ISREG(statInfo.st_mode))
			{
				break;
			}
		}
		m_currFilename=pDirInfo->d_name;
		syslog(LOG_ERR,"read msg: %s", pDirInfo->d_name);
		if (m_bufSize<statInfo.st_size)
		{
			delete[] m_pDataBuf;
			m_bufSize=statInfo.st_size;
			m_pDataBuf=new char[m_bufSize];
			if (m_pDataBuf==NULL){
				syslog(LOG_ERR,"memory alloc in getNextSMSFromStorage failed: %s", pDirInfo->d_name);
			}
		}
		m_dataSize=statInfo.st_size;
		std::ifstream ifs;
		ifs.exceptions(std::ios::badbit|std::ios::failbit);
		try
		{
			std::string path=m_IncomingDirectory+"/"+pDirInfo->d_name;
			ifs.open(path.c_str());
			ifs.read(m_pDataBuf,m_dataSize);
			ifs.close();
		}
		catch (std::exception e)
		{
			std::string errorMsg="read ";
			errorMsg+=pDirInfo->d_name;
			errorMsg+="error :" ;
			errorMsg+=e.what();
			throw SMS_Storage_error(errorMsg);			
		}
		return 0;

	};

	int getFirstSMSFromStorage() {
		m_pDIR=opendir(m_IncomingDirectory.c_str());
		if (m_pDIR==NULL)
		{
			std::string errorMsg="open dir ";
			errorMsg+=m_IncomingDirectory;
			errorMsg+="error" ;
			throw SMS_Storage_error(errorMsg);		
		}
		syslog(LOG_ERR,"open dir successful: %s",m_IncomingDirectory.c_str()); 
		return getNextSMSFromStorage();

	}

	int readGettedSMS(char* buf, unsigned int* buf_size) {
		if (*buf_size < m_dataSize)
		{
			*buf_size=m_dataSize;
			return 0;
		}
		memcpy(buf,m_pDataBuf,m_dataSize);
		*buf_size=m_dataSize;
		return 0;
	}

	int recordSended(){
			m_readedFileList.push_back(m_currFilename);
	}

	int backupError() {
		std::string oldpath(m_IncomingDirectory);
		std::string newpath(BACKUPDIR);
		oldpath+="/"+m_currFilename;
		newpath+="/"+m_currFilename;
		link(oldpath.c_str(),newpath.c_str());
		unlink(oldpath.c_str());
		return SUCCESS;
	}

	int clearStorage(){
		std::vector<std::string>::const_iterator it;
		for (it=m_readedFileList.begin();it!=m_readedFileList.end();it++){
			std::string file=m_IncomingDirectory+"/"+(*it);
			unlink(file.c_str());
			syslog(LOG_ERR,"deleted file:%s", file.c_str());
		}
		m_readedFileList.clear();
		closedir(m_pDIR);
		m_pDIR=NULL;
		return 0;
	}

};

}

#endif
