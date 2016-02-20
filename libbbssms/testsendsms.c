// NJU bbssms, preview version, zhch@dii.nju.edu.cn, 2000.3.23 //
// HIT bbssms, Changed by Sunner, sun@bbs.hit.edu.cn, 2000.6.11
// zixia bbssms, Changed by zdh, dh_zheng@hotmail.com,2001.12.04
// zixia bbssms, Changed by zixia, zixia@zixia.net, 2002.12.25
// zixia pandora, Changed by zixia, zixia@zixia.net, 2003.1.25
// zixia pandora, Change by roy, roy@zixia.net 2003.6.08
#include "bbs.h"
#include "select.h"
#include "tcplib.h"
#include <netdb.h>

#define TIME_OUT	15
#define MAX_PROCESS_BAR_LEN 30
#define COMMANDNUM	5

char * commands[]={"连接网关","注册手机", "发送短信", "取消手机注册","退出"} ;
int sockfd;
jmp_buf jb;

extern char fromhost[];
extern struct userec *currentuser;
extern int msg_count;
extern struct user_info uinfo;
extern int utmpent;

struct _select_def bbssms_conf;

static int
doconnect(){
	return SHOW_REFRESH; 
};

static int
doregister(){
	return SHOW_REFRESH; 
};

static int
dosendsms(){
	return SHOW_REFRESH; 
};

static int
dounregister(){
	return SHOW_REFRESH; 
};

static int
doexit(){
	if (sockfd>=0)
		close(sockfd);
	return SHOW_QUIT; 
};

typedef static int (*func)();
func commandlist[]={doconnect, doregister, dosendsms, dounregister, doexit};

static void 
locate(int n)
{
    int x, y;
    char buf[20];

    if (n >= bbssms_conf.item_count)
        return;
    y = n+2  ;
    x = 10;
	
	move(y, x);
}	


static bool bbssms_redraw=true;

static time_t last_refresh;

	} else 	
		return SHOW_REFRESH;
}

static int bbssms_onselect(struct _select_def *conf)
{
	bbssms_redraw=true;
	return commandlist[conf->pos-1]();
}

static int bbssms_show(struct _select_def *conf, int pos)
{
    return SHOW_CONTINUE;
}

static int bbssms_key(struct _select_def *conf, int command)
{
	int pos=command-'0';
    if ( (pos>=0) && (pos<COMMANDNUM)){
	  conf->new_pos = pos;
	  return SHOW_SELCHANGE;
    }
    switch (command) {
    case Ctrl('C'):
    case Ctrl('A'):
    	   return SHOW_QUIT;
    case '$':
    	  conf->new_pos=conf->item_count;
    	  return SHOW_SELCHANGE;
    case '^':
    	  conf->new_pos=1;
    	  return SHOW_SELCHANGE;
    }
    return SHOW_CONTINUE;
}

static void clear_information(){
	move(11,2);
	clrtoeol();
	move(12,2);
	clrtoeol();
}

static int bbssms_selchange(struct _select_def* conf,int new_pos)
{
    static oldn = -1;

    if (oldn >= 0) {
        locate(oldn);
        prints("[1;32m %d.[m%s", oldn, commands[oldn]);
    }
    oldn = new_pos-1;

    locate(new_pos-1);
    prints("[%d][1;42m%s[m", new_pos-1, commands[new_pos-1]);

	switch(new_pos) {
		case 1:
			clear_information();
			if (sockfd<0) {
				move(11,2);
    			prints("[1;37m网关登录id：[1;33m__________________________");
				move(12,2);
    			prints("[1;37m登录密码：[1;33m__________________________");
			} else {
				move(11,2);
				prints("您已经和网关连接上了！");
			}
			break;
		case 2:
			clear_information();
			if (sockfd<0) {
				move(11,2);
				prints("您尚未与网关建立连接");
			} else {
				move(11,2);
    			prints("[1;37m绑定的手机号：[1;33m__________________________");
				move(12,2);
    			prints("[1;37m注册码：[1;33m__________________________");
			}
			break;
		case 3:
			clear_information();
			if (sockfd<0) {
				move(11,2);
				prints("您尚未与网关建立连接");
			} else {
				move(11,2);
	    		prints("[1;37m目标手机号：[1;33m__________________________");
				move(12,2);
    			prints("[1;37m短信内容：[1;33m_________________________________________");
			}
			break;
		case 4:
			clear_information();
			if (sockfd<0) {
				move(11,2);
				prints("您尚未与网关建立连接");
			} else {
				move(11,2);
	    		prints("[1;37m取消绑定的手机号：[1;33m__________________________");
				move(12,2);
			}
			break;
		case 5:
			clear_information();
			move(11,2);
			prints("退出测试系统");
			break;
	}
    return SHOW_CONTINUE;
}

static void bbssms_refresh(struct _select_def *conf)
{

	int n;
	clear();
    prints("━━━━━━━━━━━━━━━━[1;35m 短  信  测  试 [m━━━━━━━━━━━━━━━━");
	move(10,0);
	prints("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
	for (n = 0; n < conf->item_count; n++) {
		locate(n);
		prints("[1;32m %d.[m%s", n, commands[n]);
	}
    bbssms_selchange(conf,conf->pos);

}


static void main_loop()
{
	int i;
	POINT pts[COMMANDNUM];

   for (i = 0; i < COMMANDNUM; i++) {
	   pts[i].x = 9;
	   pts[i].y = i+2;
   };
	bzero(&bbssms_conf,sizeof(bbssms_conf));
	bbssms_conf.flag = LF_FORCEREFRESHSEL | LF_BELL | LF_LOOP;     //|LF_HILIGHTSEL;
	bbssms_conf.prompt = NULL;
	bbssms_conf.item_pos = pts;
	bbssms_conf.arg = NULL;
	bbssms_conf.title_pos.x = 0;
	bbssms_conf.title_pos.y = 0;
	bbssms_conf.pos = 1;
	bbssms_conf.page_pos = 1;

	bbssms_conf.on_select = bbssms_onselect;
	bbssms_conf.show_data = bbssms_show;
	bbssms_conf.key_command = bbssms_key;
	bbssms_conf.show_title = bbssms_refresh;
	bbssms_conf.get_data = NULL;
	bbssms_conf.on_selchange = bbssms_selchange;
	bbssms_conf.item_count= COMMANDNUM;
	bbssms_conf.item_per_page = bbssms_conf.item_count;

	list_select_loop(&bbssms_conf);
}



int testsendsms()
{
	sockfd=-1;
	modify_user_mode(BBSNET);

    main_loop();
	return 0;
}
