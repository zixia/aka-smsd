
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the GW_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// GW_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef __cplusplus
extern "C" {
#endif
#define BIND			0x00000001
#define BINDRESP		0x80000001
#define UNBIND			0x00000002
#define UNBINDRESP		0x80000002
#define SUBMIT			0x00000004
#define SUBMITRESP		0x80000004
#define DELIVERY		0x00000005
#define DELIVERYRESP		0x80000005
#define QUERY			0x00000006
#define QUERYRESP 		0x80000006
#define CANCEL			0x00000007
#define CANCELRESP		0x80000007
#define ALIVE			0x00000008
#define ALIVERESP		0x80000008
#define IS5168 1
#if IS5168
#define VERSION 0x10
#else
#define VERSION 0x20
#endif
#define LEADVERSION 0x20
#define DWORD unsigned long
#define WORD unsigned short
#define BYTE unsigned char

struct CLogin
{
	char uid[21];
	char pwd[16];
};
struct CSubmit
{
	char mobile[21];
	char service_id[10];//027001999
#if IS5168
	char src_term[21];
	char fee_term[21];
#endif
	char msg[160];
	char udhi,pid,isReply,msg_len;
	DWORD msg_id1,msg_id2;
	BYTE msg_fmt;
};
struct CLogin_resp
{
	DWORD result;
};
struct CSubmit_resp
{
	DWORD msg_id1,msg_id2;
	DWORD result;
};
struct CHead
{
	DWORD dwCmdID;
	DWORD dwMsgID;
};
struct CDeliver
{
	DWORD id_time,id_sn;
	BYTE isMsg;
	char mobile[21];
	char msg[160];
	BYTE msg_fmt;
	char dst_num[21];
	DWORD status;
	char sub_time[30];
	char done_time[30];
};
struct CDel_resp
{
	DWORD id_time,id_sn;
	DWORD result;
};
#if IS5168

typedef struct
{
        char    service_id[11];
        char    query_time[9];
        DWORD             nMT_TLMsg;
        DWORD             nMT_TLusr;
        DWORD             nMT_Scs;
        DWORD             nMT_WT;
        DWORD             nMT_FL;
        DWORD             nMO_Scs;
        DWORD             nMO_WT;
        DWORD             nMO_FL;
}       CQueryResp;
typedef struct
{
        char    day[9];
        char    service_id[11];
}       CQueryMsg;

#endif
struct CMsg
{
	CHead head;
	union
	{
		CLogin lg;
		CSubmit sb;
		CDel_resp dr;
#if IS5168
                CQueryMsg qm;
#endif
	};
};

struct CResp
{
	CHead head;
	union
	{
		CLogin_resp lr;
		CSubmit_resp sr;
		CDeliver dl;
#if IS5168
                CQueryResp qr;
#endif
	};
};

void apiStop();
DWORD apiLogin(const char*,WORD,const char*,const char*);
DWORD apiActive();
#if IS5168
DWORD apiSend(
	DWORD msg_id1,
	DWORD msg_id2,
	char mobile[21],
	char service_id[10],
	char src_term[21],
	char fee_term[21],
	char msg[160],
	char udhi,
	BYTE pid,
	BYTE isReply,
	WORD msg_len,
	BYTE msg_fmt);
DWORD apiQuery(const char* day,const char* svc);
#else
DWORD apiSend(
	DWORD msg_id1,
	DWORD msg_id2,
	char mobile[21],
	char service_id[10],
	char msg[160],
	char udhi,
	BYTE pid,
	BYTE isReply,
	WORD msg_len,
	BYTE msg_fmt);
#endif
DWORD apiRecv(CResp *msg,DWORD msecond);

#ifdef __cplusplus
}
#endif

