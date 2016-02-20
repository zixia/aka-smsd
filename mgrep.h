#ifndef MGREP_6BA4F54F_8745_43C9_8EE1_BC83A35F4ED0
#define MGREP_6BA4F54F_8745_43C9_8EE1_BC83A35F4ED0

#define MAXPAT  256
#define MAXLINE 1024
#define MAXSYM  256
#define MAXMEMBER1 4096
#define MAXPATFILE 2600 /*pattern文件的最大长度*/
#define BLOCKSIZE  8192  /*用于预读的数据大小*/
#define MAXHASH    512  /*pattern使用的hash表大小*/
#define mm 	   511  /*用于hash值的取模运算*/
#define max_num    200 /*最大的pattern个数*/
#define W_DELIM	   128
#define L_DELIM    10

extern int ONLYCOUNT, FNAME, SILENT, FILENAMEONLY, num_of_matched;
extern int INVERSE;
extern int WORDBOUND, WHOLELINE, NOUPPER;
extern unsigned char *CurrentFileName;
extern int total_line;

void default_setting();
int prepf(int fp,void** patternbuf,size_t* patt_image_len);
int mgrep(int fp,void* patternbuf);
int mgrep_str(char* data,int len,void* patternbuf);
void releasepf(void* patternbuf);

#endif

