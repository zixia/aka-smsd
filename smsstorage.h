#ifndef SMS_1A99E4FF_2AF0_45b3_B39A_592D08C9B107
#define SMS_1A99E4FF_2AF0_45b3_B39A_592D08C9B107

#include "sms.h"

namespace SMS{

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

public:
	CSMSStorage(CSMSProtocol *pSMSPProtocol):m_pSMSPProtocol(pSMSPProtocol){
	}

	int init() {
				
	return	set_notifier();
	}
	virtual ~CSMSStorage() {};


	virtual int writeSMStoStorage(const char* sourceNo, const char* TargetNo, char* buf, unsigned int buf_size)=0;

	virtual int readGettedSMS(char* buf, unsigned int* buf_size) = 0;
	
	virtual int getNextSMSFromStorage()=0;

	virtual int getFirstSMSFromStorage()=0;

	virtual int clearStorage()=0;

	int OnNotify();

};


}
#endif

