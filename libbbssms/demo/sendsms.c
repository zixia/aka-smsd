#include <sys/ipc.h>

#include <sys/shm.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <assert.h>

#include <stdio.h>

#include "sendsms.h"

static void * smsbuf=NULL;
static int smsresult=0;
static struct BBS_sms_shm_head * bbs_head;

static void *attach_shm1(int shmkey, int shmsize, int *iscreate,int readonly ,void* shmaddr)
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
static void *attach_shm(int shmkey, int shmsize, int *iscreate)
{
    return attach_shm1(shmkey, shmsize, iscreate, 0, NULL);
}

int sms_init_memory()
{
    void * p;
    int iscreate;
    if(smsbuf) return 0;

    iscreate = 0;
    p = attach_shm(8914, SMS_SHM_SIZE+sizeof(struct BBS_sms_shm_head), &iscreate);
    bbs_head = (struct BBS_sms_shm_head *) p;
    smsbuf = p+sizeof(struct BBS_sms_shm_head);
    printf("%p\n",smsbuf);
}

void sendtosms(void * n, int s)
{
    if(bbs_head->length+s>=SMS_SHM_SIZE) return;
    memcpy(smsbuf+bbs_head->length, n, s);
    bbs_head->length+=s;
}

void SMS_request(int signo)
{
    smsresult=1;
}

void SMS_request2(int signo)
{
    smsresult=-1;
}

int wait_for_result()
{
    int count;
    char fn[80];
    int i,j;
    bbs_head->sem=0;

    smsresult=0;

    signal(SIGUSR2, SMS_request2);	
    signal(SIGUSR1, SMS_request);
    count=0;
    while(!smsresult) {
#ifdef BBSMAIN
        move(t_lines-1, 0);
        clrtoeol();
        prints("发送中....%d%%", count*100/30);
        refresh();
#endif
        sleep(1);
        count++;
        if(count>30) {
#ifdef BBSMAIN
            move(t_lines-1, 0);
            clrtoeol();
#endif
            return -1;
        }
    }
#ifdef BBSMAIN
    move(t_lines-1, 0);
    clrtoeol();
#endif
	return smsresult;
}

int DoReg(char * phone, char* id)
{
    int count=0;
    struct BBS_header h;
    struct BBS_RegMobileNoPacket h1;
    h.Type = BBS_CMD_REG;
    h.pid=getpid();
    h.BodyLength=sizeof(h1);
    strcpy(h1.phone, phone);
    strcpy(h1.id, id);
    while(bbs_head->sem) {
        sleep(1);
        count++;
        if(count>=5) return -1;
    }
    bbs_head->sem=1;
    bbs_head->total++;
    sendtosms(&h, sizeof(h));
    sendtosms(&h1,  sizeof(h1));
    return wait_for_result();
}

int DoUnReg(char * phone, char* id)
{
    int count=0;
    struct BBS_header h;
    struct BBS_UnRegPacket h1;
    h.Type = BBS_CMD_UNREG;
    h.pid=getpid();
    h.BodyLength=sizeof(h1);
    strcpy(h1.phone, phone);
    strcpy(h1.id, id);
    while(bbs_head->sem) {
        sleep(1);
        count++;
        if(count>=5) return -1;
    }
    bbs_head->sem=1;
    bbs_head->total++;
    sendtosms(&h, sizeof(h));
    sendtosms(&h1, sizeof(h1));
    return wait_for_result();
}

int DoCheck(char * phone, char* id, char * code)
{
    int count=0;
    struct BBS_header h;
    struct BBS_CheckMobileNoPacket h1;
    h.Type = BBS_CMD_CHECK;
    h.pid=getpid();
    h.BodyLength=sizeof(h1);
    strcpy(h1.phone, phone);
    strcpy(h1.code, code);
    strcpy(h1.id, id);

    while(bbs_head->sem) {
        sleep(1);
        count++;
        if(count>=5) return -1;
    }
    bbs_head->sem=1;
    bbs_head->total++;
    sendtosms(&h, sizeof(h));
    sendtosms(&h1, sizeof(h1));
    return wait_for_result();
}

int DoSendSMS(DWORD uid, char * source, char* id, char * dest, char * content)
{
    int count=0;
    struct BBS_header h;
    struct BBS_BBSSendSMS h1;
    h.Type = BBS_CMD_BBSSEND;
    h.pid=getpid();
    h.BodyLength=sizeof(h1);
    h1.uid=uid;
    strcpy(h1.phone, source);
    strcpy(h1.targetphone, dest);
    strcpy(h1.id, id);
	strcpy(h1.content,content);
	printf("%s",h1.content);
    while(bbs_head->sem) {
        sleep(1);
        count++;
        if(count>=5) return -1;
    }
    bbs_head->sem=1;
    bbs_head->total++;
    sendtosms(&h, sizeof(h));
    sendtosms(&h1, sizeof(h1));
    return wait_for_result();
}

void print_help( char * filename ){
	printf ( "usage:\n"
			"\t%s <cmd> MobilePhoneNumber BBSUserID\n"
			"\t <cmd>:\n"
			"\t\tRegister: 注册手机号，获取注册码\n"
			"\t\tUnRegister: 取消注册手机号\n"
			"\t\tCheckRegisterCode: 输入注册码，执行手机绑定操作\n"
			"\t\tSendSMS: 通过这个用户发送短信\n"
			"\t\tGetMobileReply: 获取手机回复的短信\n"
			,filename
		   );
	exit ( -1 );

}

void doregister(char* phone,char* bbsuserid){
	int ret;
	if ((DoReg(phone, bbsuserid))!=1) 
		printf("注册码发送失败或已经绑定！\n");
	else 
		printf("注册码发送成功");
}

void docheckregister(char* phone,char* bbsuserid){
	int ret;
	char code[80];
	printf("请输入注册码：");
	scanf("%s", code);
	if (DoCheck(phone, bbsuserid, code)!=1)
		printf("绑定失败！\n");
	else 
		printf("绑定成功");
}

void dosendsms(char* phone,char* bbsuserid){
	int ret;
	char targetNumber[80];
	char msg[500];
	printf("请输入对方手机号码:");
	scanf("%s", targetNumber);
	printf("请输入短信内容：");
	scanf("%s",msg);
	if (DoSendSMS(1234, phone, bbsuserid, targetNumber, msg)!=1)
		printf("短信发送失败！\n");
	else 
		printf("发送成功");
}

void dounregister(char* phone,char* bbsuserid){
	int ret;
	if ((DoUnReg(phone, bbsuserid))!=1) 
		printf("取消注册失败\n");
	else 
		printf("取消注册成功");
}

int main(int argc, char** argv){
	char *phone,*bbsuserid;
	sms_init_memory();
	if (argc!=4) {
	   print_help( argv[0] );
	   return -1;
	}
	phone=argv[2];
	bbsuserid=argv[3];
	switch (argv[1][0]) {
	case 'R':
		doregister(phone,bbsuserid);
		break;
	case 'C':
		docheckregister(phone,bbsuserid);
		break;
	case 'S':
		dosendsms(phone,bbsuserid);
		break;
	case 'U':
		dounregister(phone,bbsuserid);
		break;
	case 'G':
		printf("在bbssmsd里直接向bbs用户发送");
		break;
	default:
		print_help( argv[0] );
	}
 	shmdt(bbs_head);	
	return 0;
}
