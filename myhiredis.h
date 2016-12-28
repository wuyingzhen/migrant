#ifndef _MY_HIREDIS_H
#define _MY_HIREDIS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>


#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>

typedef void  (*redisReplyCallbackFn)( redisReply *reply, void* pridata);

//int cache_search(struct pigeon* client);
int redis_connect(redisAsyncContext *conn, struct event_base *base,char *ip , int port );
void redisCommandReplyCheckCallback(  redisReply *reply,  int type , redisReplyCallbackFn  func , void* arg, bool nofree);
void redisCommandReplyCheck( redisReply *reply );


#endif