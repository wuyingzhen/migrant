/*************************************************************
 *    多线程日志系统
 *  by wuyingzhen@hotmail.com
 
**************************************************************/

#define _UP_ERRLOG_

#include "readini.h"
#include <stdio.h>
#include <stdlib.h>

#include <stdarg.h>

#include <sys/stat.h> /* for stat , off_t */
#include <sys/dir.h>  /* for MAXNAMLEN */

#include <fcntl.h>

#include <unistd.h>

#include <string.h>
#include <pthread.h>


#ifndef MAXNAMLEN
#define MAXNAMLEN   80
#endif

#ifndef UP_MAX_FILESIZE
#define UP_MAX_FILESIZE   10000000
#endif

#define LOG_FILE_NAME   "migrant"

static pthread_mutex_t  mutex_getenv = PTHREAD_MUTEX_INITIALIZER;
int g_trsmit_loglvl;

 pthread_key_t  pthr_key;

char * GetSysDate()
{
    time_t tm;
    static char systime[20];
    memset(systime,0x00,sizeof(systime));
    tm = time(NULL);
    strftime(systime, 20, "%F", localtime(&tm));
    return systime;
}

char * GetSysTime()
{
    time_t tm;
    static char systime[20];
    
    memset(systime,0x00,sizeof(systime));
    tm = time(NULL);
    //strftime(systime, 20, "%H:%M:%S", localtime(&tm));
    strftime(systime, 20, "%X", localtime(&tm));
    
    return systime;
}
void getenv_thr(char *pszEnv,char *pszEnvValue)
{
    char *p;
    pthread_mutex_lock( &mutex_getenv );
    p=getenv(pszEnv);
    if(p!=NULL)
    {
        strcpy(pszEnvValue,p);
    }
    pthread_mutex_unlock( &mutex_getenv);
}
int WriteLog_thr( int loglev, char *filename, int line, char *fmt, ... )
{
    pthread_t     tid;
    va_list args;
    FILE    *logfp;
    struct stat    fstat;
    char    *fopntype = "a";
    char    filep[ MAXNAMLEN ] ;
    char    filebak[ MAXNAMLEN ] ;
    char    path[MAXNAMLEN];
    char    strBlank[10] ;
    char    strLine[256] ;
    int     logl;
    char    cmd[80];
    int        fd;
    char    logfile[56];
    char    szLogNO[128+1] ;
    long *pslLogNo=NULL;
    char    env_hostid[10];

    tid = pthread_self();
    pthread_detach( tid );
    
    // 日志级别
    logl=g_trsmit_loglvl;
    if (logl > loglev) 
        return(0);
        
    // 日志目录
    memset(path,0x00,sizeof(path));
    getenv_thr("MIGRANT_LOGDIR",path);
    if ( strlen(path)==0 )
    {
        char log_path_tmp[1024+1] = {};
        if( -1 == getINIT("log","path",log_path_tmp) )
            sprintf(path,"%s/log","/data/cts/wuyz/src");
        sprintf(path,"%s/log",log_path_tmp);
    }
    
    memset(env_hostid,0x00,sizeof(env_hostid));
    getenv_thr("LDBALID",env_hostid);
    
    
    memset(logfile,0x00,sizeof(logfile));
    strcpy(logfile, LOG_FILE_NAME);

    /* 日志文件名由调用者动态指定 */
    sprintf( filep, "%s/%s_%ld.log", path, logfile, pthread_self() ) ;

    /* 如果文件不存在或超过指定大小 ，则重建  */
    if ( ! stat ( filep, &fstat ) && fstat.st_size > UP_MAX_FILESIZE ) 
    {
        
        rename(filep,filebak);
        fopntype = "w+";
    }

    logfp = fopen( filep, fopntype );

    if ( ( logfp == NULL ) && ( logfp = stderr ) == NULL )
    {
        fprintf( stderr, "Log File [%s] open Error!\n", filep );
        return -1;
    }

    setbuf( logfp , (char *)NULL ) ;

    sprintf(strLine, "%s %s:%d %s", "[" , filename , line , "]" );
    switch( ( strlen( strLine ) + 4 ) /4 )
    {
        case 1 :
            strcpy( strBlank , "\t\t\t\t\t\0" ) ;
            break ;
        case 2 :
            strcpy( strBlank , "\t\t\t\t\0" ) ;
            break ;
        case 3 :
            strcpy( strBlank , "\t\t\t\0" ) ;
            break ;
        case 4 :
            strcpy( strBlank , "\t\t\0" ) ;
            break ;
        case 5 :
            strcpy( strBlank , "\t\0" ) ;
            break ;
        default :
            strcpy( strBlank , "\0" ) ;
            break ;
    }

    /* 为避免写入混乱将文件加锁 */
    fd = fileno(logfp);
    lockf(fd, F_LOCK, 0l);

    /* 格式串可以包含前导换行符 */
    while( *(fmt++) == '\n' ) fprintf(logfp, "\n");
    fmt-- ;
    memset(szLogNO,0x00,sizeof(szLogNO));
    pslLogNo = (long*)pthread_getspecific( pthr_key );

    /*日志号*/
    //sprintf(szLogNO,"%s%d-%08ld",env_hostid,tid,(long*)pslLogNo);
  //  fprintf(logfp, "%s%s│%s│=", strLine , strBlank,szLogNO) ;
   
    sprintf(szLogNO,"%s %s",GetSysDate(),GetSysTime());
  	fprintf(logfp, "%s-%s : ", szLogNO,strLine) ;

    va_start( args , fmt );
    vfprintf(logfp, fmt, args);
    va_end( args );

     fprintf(logfp, "\n");

  /* 解锁 */
    lockf(fd, F_ULOCK, 0l);

    if ( logfp != stderr )
        fclose ( logfp );

    return 0;
}

