#ifndef SMS_1A99E4FF_2AF0_45b3_B39A_592D08C9B107
#define SMS_1A99E4FF_2AF0_45b3_B39A_592D08C9B107

#include "sms.h"
#include "mgrep.h"
#include "sys/file.h"

namespace SMS{

#define FILTERINTERVAL (600)

class SMS_Storage_error : public std::runtime_error{
public:
	 SMS_Storage_error(const std::string& whatString): std::runtime_error(whatString) {};

};

class CSMSStorage{

	//general class
/*
 * 1、进行 incoming 目录的 dnotify
 * 2、如果 incoming 目录中有新文件，则全部读入 CSMSStorage 的内存队列
 * 3、提供接口将内存队列中的 SMS 传递给 CSMSDaemon
 * 4、如果 CSMSDaemon 处理完毕，则由 CSMSDaemon 调用 CSMSStorage 的删除接口，
 *	删除 queue 目录中发送完毕的 SMS 文件
 * 5、提供接口，将要发送的 SMS 写入 Storage 的 outgoing 目录
 */
 protected:
	CSMSProtocol	*m_pSMSPProtocol;
	virtual int set_notifier()=0;
	char* m_buffer;
	void* m_filterBuf;
	time_t m_ft;
private:
	int initFilter(){
		time_t tm=m_ft;
		m_ft=time(NULL);
		if (m_ft-tm<=FILTERINTERVAL) {
			return 0;
		}
	    int fp;
	    size_t pattern_imagesize;
	    default_setting();
		free(m_filterBuf);
	    fp = open(SMSHOME "etc/badword", O_RDONLY);
	    if (fp==-1) {
			syslog(LOG_ERR,"can't open badword file!");
			return -1;
		}
		flock(fp,LOCK_EX);
		prepf(fp,&m_filterBuf,&pattern_imagesize);

		flock(fp,LOCK_UN);
		close(fp);
		return 0;
			
	}

public:
	CSMSStorage(CSMSProtocol *pSMSPProtocol):m_pSMSPProtocol(pSMSPProtocol),m_filterBuf(NULL),m_ft(0){
	}

	int init() {
		initFilter();
		return	set_notifier();
	}
	virtual ~CSMSStorage() {};


	virtual int writeSMStoStorage(const char* sourceNo, const char* TargetNo, char* buf, unsigned int buf_size);

	virtual int readGettedSMS(char* buf, unsigned int* buf_size) = 0;
	
	virtual int getNextSMSFromStorage()=0;

	virtual int getFirstSMSFromStorage()=0;

	virtual int clearStorage()=0;

	virtual int recordSended()=0;

	virtual int backupError()=0;

	int OnNotify();

};


}
#endif

