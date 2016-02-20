/* Copyright (c) 1991 Sun Wu and Udi Manber.  All Rights Reserved. */
/* multipattern matcher */
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mgrep.h"
#include "sms.h"

int ONLYCOUNT, FNAME, SILENT, FILENAMEONLY, num_of_matched;
int INVERSE;
int WORDBOUND, WHOLELINE, NOUPPER;
unsigned char *CurrentFileName;
int total_line;

struct pat_list {
    int index;
    int next;
/*    struct pat_list *next;*/
};
struct pattern_image {
	int LONG;
	int SHORT;
	int p_size;
	unsigned char SHIFT1[MAXMEMBER1];
	unsigned char tr[MAXSYM];
	unsigned char tr1[MAXSYM];
        unsigned int HASH[MAXHASH];
	unsigned char buf[MAXPATFILE + BLOCKSIZE];
	unsigned char pat_spool[MAXPATFILE + 2 * max_num + MAXPAT];
	unsigned long patt[max_num]; /*用于指向pat_spool的偏移*/
	unsigned char pat_len[max_num];
	struct pat_list hashtable[max_num];
};

void default_setting()
{
    WHOLELINE = 0;
    NOUPPER = 1;
    INVERSE = 0;
    FILENAMEONLY = 1;
    WORDBOUND = 0;
    SILENT = 1;
    FNAME = 1;
    ONLYCOUNT = 0;

    num_of_matched = 0;
}

void releasepf(void* buf)
{
	struct pattern_image* patt_img=(struct pattern_image*)buf;
    int i;
    /*
    for (i = 0; i < MAXHASH; i++) {
        struct pat_list* curr;
        curr=patt_img->HASH[i];
        while (curr!=NULL) {
            struct pat_list* next;
            next=curr->next;
        	free((void*)curr);
        	curr=next;
        }
    }
    */
    free((void*)patt_img);
}

static void f_prep(int pat_index, unsigned char * Pattern,struct pattern_image* patt_img)
{
    int i, j, m;
    register unsigned hash, Mask = 15;
	struct pat_list *pt, *qt;

    m = patt_img->p_size;
    for (i = m - 1; i >= (1 + patt_img->LONG); i--) {
        hash = (Pattern[i] & Mask);
        hash = (hash << 4) + (Pattern[i - 1] & Mask);
        if (patt_img->LONG)
            hash = (hash << 4) + (Pattern[i - 2] & Mask);
        if (patt_img->SHIFT1[hash] >= m - 1 - i)
            patt_img->SHIFT1[hash] = m - 1 - i;
    }
    if (patt_img->SHORT)
        Mask = 255;             /* 011111111 */
    hash = 0;
    for (i = m - 1; i >= 0; i--) {
        hash = (hash << 4) + (patt_img->tr[Pattern[i]] & Mask);
    }
/*
	if(INVERSE) hash = Pattern[1];
*/
    hash = hash & mm;
    qt = &patt_img->hashtable[pat_index-1];
    qt->index = pat_index;
    pt = &patt_img->hashtable[patt_img->HASH[hash]-1];
    qt->next = pt->index;
    patt_img->HASH[hash] = pat_index;
}

int prepf(int fp,void** patternbuf,size_t* patt_image_len)  //读取pattern文件
{
	struct pattern_image** ppatt_img=(struct pattern_image**)patternbuf;
    int length = 0, i, p = 1, pdx = 0, num_pat;
    struct pattern_image *patt_img;
    unsigned char *pat_ptr , temp[10];
    unsigned Mask = 15;
    int num_read;

    *ppatt_img=(struct pattern_image*)malloc(sizeof(struct pattern_image));
    patt_img=*ppatt_img;
    *patt_image_len=sizeof(*patt_img);
    bzero(patt_img,*patt_image_len);
    pat_ptr=patt_img->pat_spool;
    patt_img->LONG = 0;
    patt_img->SHORT = 0;
    patt_img->p_size = 0;
    while ((num_read = read(fp, patt_img->buf + length, BLOCKSIZE)) > 0) {
        length = length + num_read;
        if (length > MAXPATFILE) {
            syslog(LOG_ERR, "maximum pattern file size is %d\n", MAXPATFILE);
            return -1;
        }
    }
    patt_img->buf[length] = '\n';
    i = 0;
    p = 1;
    while (i < length) {
        patt_img->patt[p] = pat_ptr-patt_img->pat_spool;
        if (WORDBOUND)
            *pat_ptr++ = W_DELIM;
        if (WHOLELINE)
            *pat_ptr++ = L_DELIM;
        while ((*pat_ptr = patt_img->buf[i++]) != '\n')
            pat_ptr++;
        if (WORDBOUND)
            *pat_ptr++ = W_DELIM;
        if (WHOLELINE)
            *pat_ptr++ = L_DELIM;       /* Can't be both on */
        *pat_ptr++ = 0;
        p++;
    }
    if (p > max_num) {
        syslog(LOG_ERR, "maximum number of patterns is %d\n", max_num);
        return -1;
    }
    for (i = 1; i < 20; i++)
        *pat_ptr = i;           /* boundary safety zone */
    for (i = 0; i < MAXSYM; i++)
        patt_img->tr[i] = i;
    if (NOUPPER) {
        for (i = 'A'; i <= 'Z'; i++)
            patt_img->tr[i] = i + 'a' - 'A';
    }
    if (WORDBOUND) {
        for (i = 0; i < 128; i++)
            if (!isalnum(i))
                patt_img->tr[i] = W_DELIM;
    }
    for (i = 0; i < MAXSYM; i++)
        patt_img->tr1[i] = patt_img->tr[i] & Mask;
    num_pat = p - 1;
    patt_img->p_size = MAXPAT;
    for (i = 1; i <= num_pat; i++) {
        p = strlen((char*) (patt_img->pat_spool+patt_img->patt[i]));
        patt_img->pat_len[i] = p;
        if (p != 0 && p < patt_img->p_size)
            patt_img->p_size = p;
    }
    if (patt_img->p_size == 0) {
        syslog(LOG_ERR, "the pattern file is empty\n");
        return -1;
    }
    if (length > 400 && patt_img->p_size > 2)
        patt_img->LONG = 1;
    if (patt_img->p_size == 1)
        patt_img->SHORT = 1;
    for (i = 0; i < MAXMEMBER1; i++)
        patt_img->SHIFT1[i] = patt_img->p_size - 2;
    for (i = 0; i < MAXHASH; i++) {
        patt_img->HASH[i] = 0;
    }
    for (i = 1; i <= num_pat; i++)
        f_prep(i, patt_img->pat_spool+patt_img->patt[i],patt_img);
}

static int m_short(unsigned char* text,int start,int end,void * buf);
static void monkey1(register unsigned char *text, int start, int end, void* buf);

int mgrep_str(char *text, int num,void* buf)  //匹配字符串
{
	struct pattern_image* patt_img=(struct pattern_image*)buf;
    if (patt_img->SHORT)
        m_short((unsigned char *)text, 0, num-1, patt_img);
    else
        monkey1((unsigned char*)text, 0, num-1, patt_img);
    return num_of_matched;
}      /* end mgrep_str */

static void countline(unsigned char *text, int len)				//计算文章行数
{
    int i;

    for (i = 0; i < len; i++)
        if (text[i] == '\n')
            total_line++;
}

int mgrep(int fd,void* buf)					//在文件中匹配
{
	struct pattern_image *patt_img=(struct pattern_image *)buf;
    register char r_newline = '\n';
    unsigned char text[2 * BLOCKSIZE + MAXLINE];
    register int buf_end, num_read, i = 0, j, start, end, residue = 0;

    text[MAXLINE - 1] = '\n';   /* initial case */
    start = MAXLINE - 1;

    while ((num_read = read(fd, text + MAXLINE, BLOCKSIZE)) > 0) {
        if (INVERSE && ONLYCOUNT)
            countline(text + MAXLINE, num_read);
        buf_end = end = MAXLINE + num_read - 1;
        while (text[end] != r_newline && end > MAXLINE)
            end--;
        residue = buf_end - end + 1;
        text[start - 1] = r_newline;
        if (patt_img->SHORT)
            m_short(text, start, end,patt_img);
        else
            monkey1(text, start, end,patt_img);
        if (FILENAMEONLY && num_of_matched) {
            return num_of_matched;
        }
        start = MAXLINE - residue;
        if (start < 0) {
            start = 1;
        }
        strncpy((char*)text + start, (char*)text + end, residue);
    }                           /* end of while(num_read = ... */
    text[MAXLINE] = '\n';
    text[start - 1] = '\n';
    if (residue > 1) {
        if (patt_img->SHORT)
            m_short(text, start, end,patt_img);
        else
            monkey1(text, start, end,patt_img);
    }
    return 0;
}                               /* end mgrep */




void monkey1(register unsigned char *text, int start, int end, void* buf)
{
	struct pattern_image* patt_img=(struct pattern_image*)buf;
    register unsigned char *textend;
    register unsigned hash, i;
    register unsigned char shift;
    register int m1, j, Long = patt_img->LONG;
    int pat_index, m = patt_img->p_size;
    int MATCHED = 0;
    register unsigned char *qx;
    register struct pat_list *p;
    unsigned char *lastout;
    int OUT = 0;

    textend = text + end;
    m1 = m - 1;
    lastout = text + start + 1;
    text = text + start + m1;
    while (text <= textend) {
        hash = patt_img->tr1[*text];
        hash = (hash << 4) + (patt_img->tr1[*(text - 1)]);
        if (Long)
            hash = (hash << 4) + (patt_img->tr1[*(text - 2)]);
        shift = patt_img->SHIFT1[hash];
        if (shift == 0) {
            hash = 0;
            for (i = 0; i <= m1; i++) {
                hash = (hash << 4) + (patt_img->tr1[*(text - i)]);
            }
            hash = hash & mm;
            p = &patt_img->hashtable[patt_img->HASH[hash]-1];
            while (p != 0) {
                pat_index = p->index;
		if (p->next==0)
		    p=NULL;
		else
                    p = &patt_img->hashtable[p->next-1];
                qx = text - m1;
                j = 0;
                while (patt_img->tr[*(patt_img->pat_spool+patt_img->patt[pat_index]+j)] == patt_img->tr[*(qx++)])
                    j++;
                if (j > m1) {
                    if (patt_img->pat_len[pat_index] <= j) {
                        if (text > textend)
                            return;
                        num_of_matched++;
                        if (FILENAMEONLY || SILENT)
                            return;
                        MATCHED = 1;
                        if (ONLYCOUNT) {
                            while (*text != '\n')
                                text++;
                        } else {
                            if (!INVERSE) {
                                if (FNAME)
                                    printf("%s: ", CurrentFileName);
                                while (*(--text) != '\n');
                                while (*(++text) != '\n')
                                    putchar(*text);
                                printf("\n");
                            } else {
                                if (FNAME)
                                    printf("%s: ", CurrentFileName);
                                while (*(--text) != '\n');
                                if (lastout < text)
                                    OUT = 1;
                                while (lastout < text)
                                    putchar(*lastout++);
                                if (OUT) {
                                    putchar('\n');
                                    OUT = 0;
                                }
                                while (*(++text) != '\n');
                                lastout = text + 1;
                            }
                        }
/*
				else {
			  		if(FNAME) printf("%s: ",CurrentFileName);
                          		while(*(--text) != '\n');
                          		while(*(++text) != '\n') putchar(*text);
			  		printf("\n");
				}
*/
                    }
                }
                if (MATCHED)
                    break;
            }
            if (!MATCHED)
                shift = 1;
            else {
                MATCHED = 0;
                shift = m1;
            }
        }
        text = text + shift;
    }
    if (INVERSE && !ONLYCOUNT)
        while (lastout <= textend)
            putchar(*lastout++);
}

int m_short(unsigned char* text,int start,int end,void * buf)
{
	struct pattern_image* patt_img=(struct pattern_image* )buf;
    register unsigned char *textend;
    register unsigned i;
    register int j;
    register struct pat_list *p, *q;
    register int pat_index;
    int MATCHED = 0;
    int OUT = 0;
    unsigned char *lastout;
    unsigned char *qx;

    textend = text + end;
    lastout = text + start + 1;
    text = text + start - 1;
    while (++text <= textend) {
        p = &patt_img->hashtable[patt_img->HASH[*text]-1];
        while (p != 0) {
            pat_index = p->index;
	    if (p->next==0)
	        p=NULL;
	    else
                p = &patt_img->hashtable[p->next-1];
            qx = text;
            j = 0;
            while (patt_img->tr[*(patt_img->pat_spool+patt_img->patt[pat_index]+j)] == patt_img->tr[*(qx++)])
                j++;
            if (patt_img->pat_len[pat_index] <= j) {
                if (text >= textend)
                    return 0;
                num_of_matched++;
                if (FILENAMEONLY || SILENT)
                    return 0;
                if (ONLYCOUNT) {
                    while (*text != '\n')
                        text++;
                } else {
                    if (FNAME)
                        printf("%s: ", CurrentFileName);
                    if (!INVERSE) {
                        while (*(--text) != '\n');
                        while (*(++text) != '\n')
                            putchar(*text);
                        printf("\n");
                        MATCHED = 1;
                    } else {
                        while (*(--text) != '\n');
                        if (lastout < text)
                            OUT = 1;
                        while (lastout < text)
                            putchar(*lastout++);
                        if (OUT) {
                            putchar('\n');
                            OUT = 0;
                        }
                        while (*(++text) != '\n');
                        lastout = text + 1;
                        MATCHED = 1;
                    }
                }
            }
            if (MATCHED)
                break;
        }
        MATCHED = 0;
    }                           /* while */
    if (INVERSE && !ONLYCOUNT)
        while (lastout <= textend)
            putchar(*lastout++);
}


