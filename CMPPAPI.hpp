/*  --------------------------------------------------------------------------
    Name:       CMPPAPI.hpp
    Title:      CMPP API of ISMG for CMPP 1.1
    Package:    ISMG for CMPP 1.1

    Written:    2001/01/01  Asiainfo
    Revised:    

    Synopsis:   Provide CMPP 1.1 API to ICP.
 -----------------------------------------------------------------------------*/

#ifndef _CMPP_API_HPP_
#define _CMPP_API_HPP_

#define MAX_SM_LEN		250

/* nNeedReply macro */
#define NEED_REPLY_YES	1
#define NEED_REPLY_NO	0

/* nMsgFormat macro */
#define MSG_FORMAT_ASCII		0
#define MSG_FORMAT_WRITE_CARD	3
#define MSG_FORMAT_BINARY		4
#define MSG_FORMAT_UCS2			8
#define MSG_FORMAT_GB			15

/* nErrorCode macro */
#define ERROR_CODE_OK						0
#define ERROR_CODE_INVALID_LEN				4
#define ERROR_CODE_INVALID_FEECODE			5
#define ERROR_CODE_TOO_LONG					6
#define ERROR_CODE_INVALID_SERVICEID		7
#define ERROR_CODE_TOO_FAST					8
#define ERROR_CODE_INVALID_ICP				10
#define ERROR_CODE_INVALID_MSGFORMAT		11
#define ERROR_CODE_INVALID_FEETYPE			12
#define ERROR_CODE_INVALID_VALIDTIME		13
#define ERROR_CODE_INVALID_ATTIME			14
#define ERROR_CODE_INVALID_SRCTERMID		15
#define ERROR_CODE_INVALID_DESTTERMID		16
#define ERROR_CODE_INVALID_DESTTERMIDFILE	17
#define ERROR_CODE_INVALID_MSGFILE			18
#define ERROR_CODE_INVALID_MSG				19
#define ERROR_CODE_CONNECT_FAIL				20
#define ERROR_CODE_LOGIN_FAIL				21
#define ERROR_CODE_GET_RESP_FAIL			22
#define ERROR_CODE_QUEUE_FULL				23
#define ERROR_CODE_EXCEED_LIMIT				24
#define ERROR_CODE_INVALID_USER_TYPE		25
#define ERROR_CODE_INVALID_FEETERMID		26
#define ERROR_CODE_SYSTEM_ERROR				99

#define QUERY_TYPE_TOTAL		0
#define QUERY_TYPE_SERVICEID	1

#define SM_STATUS_IN_QUEUE			0
#define SM_STATUS_SEND_SMC_OK		1
#define SM_STATUS_SEND_SMC_FAIL		2
#define SM_STATUS_USER_RECV_OK		3
#define SM_STATUS_USER_NO_RECV		4

typedef struct
{
	char	sMsgID[21+1];
	int		nErrorCode;
	char	sPhoneNo[21+1];
}	SendBatchResp;

typedef struct
{
	int		nMT_TLMsg;
	int		nMT_TLusr;
	int		nMT_Scs;
	int		nMT_WT;
	int		nMT_FL;
	int		nMO_Scs;
	int		nMO_WT;
	int		nMO_FL;
}	QueryResp;

typedef struct
{
	char	sMsgID[21+1];
	int		nMsgLevel;
	char	sServiceID[10+1];
	int		nMsgFormat;
	char	sSrcTermID[21+1];
	int		nIsReply;	/* 0/1 */
	int		nMsgLen;
	char	sMsgContent[MAX_SM_LEN+1];
	char	sDestTermID[21+1];
	char	cTpPid;
	char	cTpUdhi;
}	DeliverResp;

/* Must call this initiate function before calling others CMPP API functions
   return: 0=OK, 1=fail
*/

// sDestNo: destination number, length=21. if sDestNo=NULL, DeliverCallbackFun
//          would not return DestNo
// return: 0=OK, 1=fail
typedef int (*DeliverCallbackFun)(DeliverResp *pDeliverResp);

#ifdef _C_COMPILER_
extern "C"
{
int InitCMPPAPI(const char *sINIFile);
}
#else
int InitCMPPAPI(const char *sINIFile = "../config/cmppc.ini");
#endif

/* send a short message to single user
   return: 0=OK, 1=fail, 
   if succeed, message id is filled in sMsgID.
   if fail, error code is filled in nErrorCode.
*/
#ifdef _C_COMPILER_
extern "C"
{
#endif

int CMPPSendSingle(const int nNeedReply, const int nMsgLevel,
		const char *sServiceID, const int nMsgFormat,
		const char *sFeeType, const char *sFeeCode,
		const char *sValidTime, const char *sAtTime,
		const char *sSrcTermID, const char *sDestTermID,
		const int nMsgLen, const char *sMsgContent,
		char *sMsgID, int *nErrorCode,
		const char cFeeUserType, const char *sFeeTerminalId,
		const char cTpPid, const char cTpUdhi);

#ifdef _C_COMPILER_

}
#endif

/* send a short message to multiple users
   return: 0=OK, 1=fail, 
   if succeed, all message ids are filled in sMsgIDFile.
   if fail, error code is filled in nErrorCode.
   the short message is passed by file.
*/
#ifdef _C_COMPILER_
extern "C"
{
#endif

int CMPPSendBatch(const int nNeedReply, const int nMsgLevel,
		const char *sServiceID, const int nMsgFormat,
		const char *sFeeType, const char *sFeeCode,
		const char *sValidTime, const char *sAtTime,
		const char *sSrcTermID, const char *sDestTermIDFile,
		const char *sMsgFile, char *sMsgIDFile,
		const char cFeeUserType, const char *sFeeTerminalId,
		const char cTpPid, const char cTpUdhi);

#ifdef _C_COMPILER_
}
#endif

/* send a short message to multiple users
   return: 0=OK, 1=fail, 
   if succeed, all message ids are filled in sMsgIDFile.
   if fail, error code is filled in nErrorCode.
   the short message is passed by string.
*/
#ifdef _C_COMPILER_
extern "C"
{
#endif

int CMPPSendBatch1(const int nNeedReply, const int nMsgLevel,
		const char *sServiceID, const int nMsgFormat,
		const char *sFeeType, const char *sFeeCode,
		const char *sValidTime, const char *sAtTime,
		const char *sSrcTermID, const char *sDestTermIDFile,
		const int nMsgLen, const char *sMsgContent,
		char *sMsgIDFile,
		const char cFeeUserType, const char *sFeeTerminalId,
		const char cTpPid, const char cTpUdhi);

#ifdef _C_COMPILER_
}
#endif

/* send a short message to multiple users
   return: 0=OK, 1=fail, 
   if succeed, all message ids are filled in sMsgIDFile.
   if fail, error code is filled in nErrorCode.
   the short message and DestTermID are passed by string.
*/
#ifdef _C_COMPILER_
extern "C"
{
#endif

int CMPPSendBatch2(const int nNeedReply, const int nMsgLevel,
		const char *sServiceID, const int nMsgFormat,
		const char *sFeeType, const char *sFeeCode,
		const char *sValidTime, const char *sAtTime,
		const char *sSrcTermID, const char *sDestTermIDs,
		const int nMsgLen, const char *sMsgContent,
		char *sMsgIDFile,
		const char cFeeUserType, const char *sFeeTerminalId,
		const char cTpPid, const char cTpUdhi);

#ifdef _C_COMPILER_
}
#endif

/* nPos start from 0
   the message id and error code are filled in pSendBatchResp
   return: 0=OK, 1=fail
*/
#ifdef _C_COMPILER_
extern "C"
{
#endif

int GetSendBatchResp(char *sMsgIDFile, int nPos, SendBatchResp *pSendBatchResp);

#ifdef _C_COMPILER_
}
#endif

/* sDate format: yyyymmdd
   nQueryType: 0=total, 1=service id
   return: 0=OK, 1=fail
   if succeed, query result is filled in pQueryResp.
*/
#ifdef _C_COMPILER_
extern "C"
{
#endif

int CMPPQuery(const char *sDate, const int nQueryType,
				const char *sServiceID, QueryResp *pQueryResp);

#ifdef _C_COMPILER_
}
#endif

/* nTimeout: how many seconds the function keep waiting short message, 0 means wait forever.
   return: 0=OK, 1=fail
   if succeed, the delivered short message is filled in pDeliverResp.
*/
#ifdef _C_COMPILER_
extern "C"
{
#endif

int CMPPDeliver(const int nTimeout, DeliverResp *pDeliverResp);

#ifdef _C_COMPILER_
}
#endif

/* pDeliverCallbackFun: call back function to process MO message
   return: 0=OK, 1=fail
   CMPPDeliverCallback will wait ISMG MO message until the connection is broken or
   pDeliverCallbackFun return 1.
*/
#ifdef _C_COMPILER_
extern "C"
{
#endif

int CMPPDeliverCallback(DeliverCallbackFun pDeliverCallbackFun);

#ifdef _C_COMPILER_
}
#endif

/* return: 0=OK, 1=fail */
#ifdef _C_COMPILER_
extern "C"
{
#endif

int CMPPCancel(const char *sMsgID);

#ifdef _C_COMPILER_
}
#endif

/* return: 0=OK, 1=fail */
#ifdef _C_COMPILER_
extern "C"
{
#endif

int CMMPActiveTest(int *nErrorCode);

#ifdef _C_COMPILER_
}
#endif

/* the status of short message is stored in nStatus
   return: 0=OK, 1=fail
*/
#ifdef _C_COMPILER_
extern "C"
{
#endif

int CMMPGetSMStatus(const char *sMsgID, int *nStatus);

#ifdef _C_COMPILER_
}
#endif

/* set ICP share key */
#ifdef _C_COMPILER_
extern "C"
{
#endif

int CMPPSetKey(const char *sKey);

#ifdef _C_COMPILER_
}
#endif

/* analyze status report content */
#ifdef _C_COMPILER_
extern "C"
{
#endif

int CMPPAnalyzeStatusReport(const char *sContent, char *sTime, char *sSMCNo, int *nMsgID,
							char *sStat, char *sSubmitTime, char *sDoneTime, 
							char *sDestTermID, int *nSMCSequence);

#ifdef _C_COMPILER_
}
#endif

#endif
