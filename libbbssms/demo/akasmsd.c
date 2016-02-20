#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#include "libbbssms.h"
#include "sendsms.h"

#define SMSGW_HOST "211.157.100.10"
#define SMSGW_PORT 50020
#define SMSGW_ID "20"
#define SMSGW_PASSWD "testbbssms"

#define SMS_SAVE_FILE "sms.txt"
void * shmbuf;
int sockfd;
int sn=0;
struct BBS_sms_shm_head* bbs_head;

void save_daemon_pid()
{
}
static void *bbs_attach_shm1(int shmkey, int shmsize, int *iscreate,int readonly ,void* shmaddr)
{
    void *shmptr;
    int shmid;

    shmid = shmget(shmkey, shmsize, 0);
    if (shmid < 0) {
        if (readonly) {
            return 0;
        }
        shmid = shmget(shmkey, shmsize, IPC_CREAT | 0660);      /* modified by dong , for web application , 1998.12.1 */
        *iscreate = 1;
        if (shmid < 0) {
            exit(0);
        }
        shmptr = (void *) shmat(shmid, shmaddr, 0);
        if (shmptr == (void *) -1) {
            exit(0);
        } else
            memset(shmptr, 0, shmsize);
    } else {
        *iscreate = 0;
        if (readonly)
            shmptr = (void *) shmat(shmid, shmaddr, SHM_RDONLY);
        else
            shmptr = (void *) shmat(shmid, shmaddr, 0);
        if (shmptr == (void *) -1) {
            exit(0);
        }
    } return shmptr;
}


static void *bbs_attach_shm(int shmkey, int shmsize, int *iscreate)
{
    return bbs_attach_shm1(shmkey, shmsize, iscreate, 0, NULL);
}
void init_memory()
{
    void * p;
    int iscreate;

    iscreate = 0;
    p = bbs_attach_shm( 8914, SMS_SHM_SIZE+sizeof(struct BBS_sms_shm_head), &iscreate);
    bbs_head = (struct BBS_sms_shm_head *) p;
    shmbuf = p+sizeof(struct BBS_sms_shm_head);
    bbs_head->sem = 0;
    bbs_head->total = 0;
    bbs_head->length = 0;
}

void start_daemon()
{
    int n;

    if (fork())
        exit(0);
    save_daemon_pid();
}
void save_sms(void *buf){
	FILE * fp;
	PSMS_GWSMS p;

	if ((fp=fopen(SMS_SAVE_FILE,"a+"))==NULL){
		return;
	}
	p=(PSMS_GWSMS)buf;

	fprintf(fp,"------------------\n");
	fprintf(fp,"接收用户id: %d\n",p->userBBSCode);
	fprintf(fp,"发送者手机号： %s \n", p->SrcMobileNo);
	fprintf(fp,"短信内容：\n%s\n",p->MsgTxt);
	fclose(fp);
}

void processremote()
{
    unsigned int pid;
	DWORD serialNo;
	int replyType;
	void* buf;
	int rc;
	if (rc=sms_getReply(&serialNo, &pid, &replyType, &buf)!=0) {
		exit(0);
	}
    switch(replyType) {
    case SMS_CMD_OK:
        kill(pid, SIGUSR1);
		break;
    case SMS_CMD_GWSMS:
	save_sms(buf);
	case SMS_CMD_REGISTER:
	case SMS_CMD_UNREGISTER:
		free(buf);
        break;
	default:
		kill(pid, SIGUSR2);
		break;

    }
}

void getbuf(void * h, int s)
{
    if(bbs_head->length<s) return;
    if(h)
        memcpy(h, shmbuf, s);
    memcpy(shmbuf, shmbuf+s, bbs_head->length-s);
    bbs_head->length-=s;
}

void processbbs()
{
    struct BBS_header h;
	struct BBS_RegMobileNoPacket h1;
	struct BBS_CheckMobileNoPacket h2;
	struct BBS_BBSSendSMS h3;
	struct BBS_UnRegPacket h4;
    if(bbs_head->sem) return;
    if(!bbs_head->total) return;
    bbs_head->sem=1;
    while(bbs_head->total) {
        bbs_head->total--;
        getbuf(&h, sizeof(h));
        switch(h.Type) {
            case BBS_CMD_REG:
		 getbuf(&h1,sizeof(h1));
		sms_sendRegisterNumber(++sn, h.pid,h1.phone, h1.id);
                break;
            case BBS_CMD_CHECK:
		 getbuf(&h2,sizeof(h2));
		sms_checkRegisterNumber(++sn, h.pid,h2.phone, h2.id, h2.code);
                break;
            case BBS_CMD_BBSSEND:
		 getbuf(&h3,sizeof(h3));
		sms_sendSMS(++sn, h.pid,h3.uid, h3.id, h3.phone, h3.targetphone, h3.content, strlen(h3.content), 0);
                break;
		case BBS_CMD_UNREG:
		 getbuf(&h4,sizeof(h4));
				sms_unRegister(++sn, h.pid,h4.phone, h4.id);
                break;
        }
    }

    bbs_head->sem=0;
}

int main()
{
    fd_set readset;
    struct timeval to;
    int rc,remain=0,retr;
	int sockfd;

	int ret;

    start_daemon();
/*
    load_sysconf();
    resolve_ucache();
    resolve_utmp();
*/
    init_memory();

	sms_init();

	//建立连接
    if ((ret=sms_login(0,SMSGW_HOST, SMSGW_PORT, SMSGW_ID, SMSGW_PASSWD))!=0){
	    printf ( "can't connect to server %s:%d use pass %s! ret no: %d\n", 
			    SMSGW_HOST, SMSGW_PORT, SMSGW_PASSWD, ret );
		exit(0);
	}
	sockfd=sms_getsocket();
    while(1) {
	FD_ZERO(&readset);
        FD_SET(sockfd, &readset);
		to.tv_sec = 1;
		to.tv_usec = 0;
        if((retr=select(sockfd+1, &readset, NULL, NULL, &to))<0) break;
        if(retr) {
            if (FD_ISSET(sockfd, &readset)) {
                    processremote();
            }
        }
        processbbs();
    }
    
    sms_exit();
    shmdt(bbs_head);
    shmbuf=NULL;
    return 0;
}

