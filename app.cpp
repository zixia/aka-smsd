#include "app.h"
#include <sys/types.h>
#include <unistd.h>

namespace RCL{

CApplication* __MainApp;
void __do_usr1(int t)
{
	RCL_GetApp()->OnSignalUser1();
}

void __doalarm(int t)
{
	RCL_GetApp()->OnSignalAlarm();
	signal(SIGALRM, __doalarm);
}

void __dohup(int t)
{
	RCL_GetApp()->OnSignalHup();
}

void __doterm(int t)
{
	RCL_GetApp()->OnSignalTerm();
}

void __doint(int t)
{
	RCL_GetApp()->OnSignalInt();
}

void __dosegv(int t)
{
	RCL_GetApp()->OnSignalSegv();
}

void __dopipeerr(int t)
{
	if (RCL_GetApp()->OnSignalSegv())
//   	signal(SIGPIPE,(void *)__dopipeerr);
   	signal(SIGPIPE, __dopipeerr);
}

void __dochild(int t)
{
	RCL_GetApp()->OnSignalChild();
    signal(SIGCHLD, __dochild);
}

void CApplication::InitDaemon()
{
	setuid(502);
	setgid(502);	
	int n;
    if(fork())
       exit(0);
    for (n = 0; n<10; n++)
        close(n);
    open("/dev/null", O_RDONLY);
    dup2(0,1);
    dup2(0,2);

    if((n=open("/dev/tty",O_RDWR)) > 0) {
    ioctl(n, TIOCNOTTY, 0) ;
       close(n);
    }

    setsid();
    if(fork())
      exit(0);
}

int CApplication::InitSignal()
{
/*        signal(SIGHUP, (void *)__dohup) ;
        signal(SIGINT, (void *)__doint);
        signal(SIGSEGV,(void *)__dosegv);

	signal(SIGPIPE,(void *)__dopipeerr);
    signal(SIGTERM,(void *)__doterm);
    signal(SIGCHLD,(void *)__dochild);
	signal(SIGUSR1,(void *)__do_usr1);
	signal(SIGALRM,(void *)__doalarm);
*/
	signal(SIGPIPE, __dopipeerr);
    signal(SIGTERM, __doterm);
    signal(SIGCHLD, __dochild);
	return 1;
}

CApplication::CApplication()
{
	__MainApp = this;
}

CApplication::~CApplication()
{
	__MainApp = NULL;
}

CApplication* RCL_GetApp()
{
	return __MainApp;
}

}


char* RCL::__z_self;

int main(int argc,char** argv,char** env)
{
	RCL::CApplication* pApp = RCL::RCL_GetApp();
	
	pApp->m_nArgc = argc;
	pApp->m_strArgv = argv;
	RCL::__z_self = argv[0];
	pApp->m_pEnv = env;

	if ( pApp->IsDaemon() )
		pApp->InitDaemon();
	pApp->InitSignal();

	pApp->Run();

	return 0;
}
