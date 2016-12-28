#ifndef _MIGRANTLOG_H
#define _MIGRANTLOG_H

/*
 *   日志函数的声明
 */ 
int WriteLog_thr( int loglev, char *filename, int line, char *fmt, ... );

#define Debug(...) WriteLog_thr(0,__FILE__,__LINE__,__VA_ARGS__);
#define AppLog(...) WriteLog_thr(1,__FILE__,__LINE__,__VA_ARGS__);
#define ErrLog(...) WriteLog_thr(2,__FILE__,__LINE__,__VA_ARGS__);



#endif