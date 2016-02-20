#ifndef SMS_H_APP_4C9557A02420417f850745DF202C81A5
#define SMS_H_APP_4C9557A02420417f850745DF202C81A5

#include "app.h"
#include <setjmp.h>

namespace SMS{

class CSMSDaemon: public RCL::CApplication{
/*
 * 1、构造函数加载 CSMSPProtocol的子类
 * 2、构造时加载 CSMSStorage 的子类
 * 3、构造时加在 CSMSPrivilige 的子类
 * 4、进行 Daemon 初始化
 * 5、使用 CSMSPProtocol 进行 Listen
 * 6、accept 后使用 CSMSPrivilige 进行认证
 * 7、 fork 子进程，如果后续有通过验证的连接，则杀死子进程
 * 8、使用 CStorage 进行 dnotify incoming 目录
 * 9、阻塞在 read socket 上
 * 10、如果有 incoming 文件到达，则 CSMSStorage 调用 CSMSDaemon 的发送函数
 */
 
protected:
	CSMSProtocol	*m_pSMSProtocol;
	CSMSStorage *m_pSMSStorage;
	jmp_buf m_jmpBuf;

	
public:
	CSMSDaemon(char* applicationName,int logFacility)
	{
		openlog(applicationName,LOG_PID,logFacility);		
	}

	int IsDaemon() {
		return 1;
	}

	int Run();

	int OnSignalChild();

	int OnSignalTerm();

	virtual ~CSMSDaemon();
	
};



}

#endif
