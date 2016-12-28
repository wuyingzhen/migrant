/***********************************************************************            
*   ini 配置文件操作类函数
*   by wuyingzhen@hotmail.com

***********************************************************************/
#ifndef __READINI_H
#define __READINI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <errno.h>
#include <time.h>
#include <sys/times.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <signal.h>
#include <setjmp.h>
#include <pthread.h>
#include <fcntl.h>
#include <assert.h>
#include <stdbool.h>

/*
#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif
*/


#ifndef MAX_STREAM
#define MAX_STREAM  4096
#endif

#ifndef TIPS_PATH_MAX
#define TIPS_PATH_MAX    1024        /*路径最大长度*/
#endif

#ifndef TIPS_LINE_MAX
#define TIPS_LINE_MAX    1024        /*文件行最大长度*/
#endif

#ifndef FLD_MAX
#define FLD_MAX     128         /*域名最大长度*/
#endif
#ifndef FLD_VAL_MAX
#define FLD_VAL_MAX 10240     
#endif


#define BUF_SIZE    4096
#define MAX_FILE_SIZE   10000000


 int readINIT ( char * );
 int getINIT ( char *, char *, char * );
 void freeINIT ();
 void FilterCommen ( char * );
int  getvalue(char *grp,char *opt);
#endif

