/******************************************************

* by   wuyingzhen@hotmial.com


******************************************************/

#include "migrant.h"
#include  "util.h"
#include "migrantlog.h"
#include <stdio.h> 
#include <string.h>
#include  <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <hiredis/hiredis.h>


#define STREAM_DATA_SIZE   13
static char* stream_data[] = {
	"Ver",          \
	"Sender",		\
	"TypeTag",	\
	"TYPE",			\
	"IIIII", 			\
	"CCCC",					\
	"OTime",					\
	"InTime",					\
	"STime",					\
	"FileName",		\
	"BBB",				\
	"priority",	\
	"UUID"
};


int save_as_file(conn *c, DATA msg_data);             // file.c



void sadd_reply(redisReply *reply, void* pridata){
    conn * c = (conn*) pridata;
    if( 1 == reply->integer ){
        to_wbuf(c,"ok",2);
        
    }
    else{       // 失败
        to_wbuf(c,"sadd error, this stream is exists",33);
    }
       

}




void hmset_reply(redisReply *reply, void* pridata){
Debug("hmset_reply begin ....");
    conn * c = (conn*) pridata;
    if( 0==  memcmp(reply->str,"OK",2) ){
            
        Debug("hmset_reply ok ....");
        
    }
    else{       // 失败
    }
}




static int save_as_redis(conn *c,DATA* msg_data,char *key){
Debug("**************************************");
Debug("[%s][%d][%s]\nkey[%s]",msg_data->data,msg_data->data_len,msg_data->data_md5,key);
        
         int  argc_t = 2*STREAM_DATA_SIZE + 4;
         char **argv_t = (char**)calloc(argc_t,sizeof(char*));
        
        size_t argvlen_t[argc_t];
        
        argvlen_t[0] = 5;
        argv_t[0] = calloc(argvlen_t[0],1);
        memcpy(argv_t[0] ,"hmset",argvlen_t[0]);

        argvlen_t[1] = 32;
        argv_t[1] = calloc(argvlen_t[1],1);
        memcpy(argv_t[1] ,msg_data->data_md5,argvlen_t[1]);
        
        argvlen_t[2] = 4;
        argv_t[2] = calloc(argvlen_t[2],1);
        memcpy(argv_t[2] ,"data",argvlen_t[2]);
        
        argvlen_t[3] =msg_data->data_len ;
        argv_t[3] = calloc(argvlen_t[3],1);
        memcpy(argv_t[3] ,msg_data->data,argvlen_t[3]);
           
        int j, i = 0;
	for(j=i+4; i < STREAM_DATA_SIZE ; ++i){
	    
	        argvlen_t[j] = strlen(stream_data[i]);
                argv_t[j] = calloc(argvlen_t[j],1);
                memcpy(argv_t[j] ,stream_data[i],argvlen_t[j]);
	    
		char *value = from_key_find_value(c->rbuf,stream_data[i]); 
		argv_t[j+1] = value;
		argvlen_t[j+1] = strlen(value);
		
		j +=2;
	}
       
	redisReply *reply =  redisCommandArgv(  c->thread->redis_conn, argc_t, argv_t ,argvlen_t );  
	redisCommandReplyCheckCallback(reply,REDIS_REPLY_STATUS,hmset_reply, c , false);
	
	reply =  redisCommand(c->thread->redis_conn," sadd %s %s", key, msg_data->data_md5);
	redisCommandReplyCheckCallback(reply,REDIS_REPLY_INTEGER,sadd_reply,c , false);
	
	
        for(i = 0;i < argc_t;++i){
            free(argv_t[i]);
        }
        free(argv_t);
            	
	
	return 0;	
}

int save_data(conn *c){
Debug(".......... save_data begin ..........\n");

	if( c->rbuf == NULL || c->rbytes == 0) return -1;
		
	// check msg : 1: msg_len  2:md5
	DATA msg_data;
Debug(" ########## check data   校对数据md5 ##########\n");
	int ret = check_msg(c->rbuf,c->rbytes,&msg_data);
	if( ret == -1){
		Debug("check msg error.");
		to_wbuf(c,"check msg error",16);
		return -1;	
	}
	Debug("check msg ok.");
	
	char *value1 =  from_key_find_value(c->rbuf,"TYPE"); 

	ret = save_as_redis(c,&msg_data,value1);
	if( ret == -1){
		Debug("save_as_redis error.");
		to_wbuf(c,"save_as_redis error",20);
		free(value1);
		return -1;	
	}
	free(value1);
/*	
	ret = save_as_file(c,msg_data);
	if( ret == -1){
		Debug("save_as_file error.");
		return -1;	
	}
*/
	return 0;
}

