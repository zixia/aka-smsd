#ifndef MOBILENUMBERLENGTH
#define MOBILENUMBERLENGTH 16  //号码长度
#endif
#define OAKSID_SM_SVRMOINFO             (0x100 + 88)

typedef struct{
        OAKSREQHEADER h;
        int  nReserve;                                          //保留字
        char szMobileNo[BUFLEN_MOBILENO];  //手机号码
        char szSPCode[20];                                 //spcode 5818后面的数字 如 1801
        int  nLenMsg;                                      //内容长度
}*POAKSREQTRANSFERMOINFO,OAKSREQTRANSFERMOINFO;            //包体后面跟着内容

const char* testport="5818";
