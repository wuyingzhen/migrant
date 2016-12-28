#include "issue.h"
#include "migrantlog.h"
#include "myhiredis.h"

static void sdiffstore_reply( redisReply *reply, void* pridata){
Debug("   sdiffstore_reply  begin ......"); 
    
}

static void sdiff_reply(redisReply *reply, void* pridata){
Debug("   sdiff_reply  begin ......");   
    conn * c = (conn*) pridata;
    if(0 == reply->elements){
    /*    memcpy(c->wbuf,"no datas!",10);
        c->wbytes = 10;
    */
        c->ileft = -1;
        freeReplyObject(reply);   // 手动释放
    }
    else{
          c->ileft = 0;
          c->sdiff_story = reply;
    }

}

static void sadd_reply(redisReply *reply, void* pridata){
    conn * c = (conn*) pridata;
    if( 1 == reply->integer ){
        c->ileft += 1;
        
    }
    else{       // 失败
        
    }
       

}

int redis_search_data(conn *c){
Debug("redis_search_data  begin ...");
    if(c->sdiff_story == NULL){
        char set_type_tmp[1024] = {};
        sprintf(set_type_tmp,"%s-%s",c->conn_client_info.types,c->conn_client_info.ip);
        Debug("sdiff   %s  %s  ", c->conn_client_info.types,  set_type_tmp);
       // Debug("type[%s],type_sent[%s]",c->conn_client_info.types,
        redisReply *reply =  redisCommand(  c->thread->redis_conn, "sdiff   %s  %s  ", c->conn_client_info.types,  set_type_tmp);  
        redisCommandReplyCheckCallback(reply,REDIS_REPLY_ARRAY,sdiff_reply,c,true);
    }
    
    if( -1 == c->ileft){
        to_wbuf(c,"no datas!\n",10);
     //   memcpy(c->wbuf,"no datas!\n",10);
     //   c->wbytes = 10;
        //c->noreply = true;
        c->noreply = false;
        
        c->write_and_go = conn_closing;
        
    }
    else{
        if( c->sdiff_story->elements == c->ileft)
        {
            to_wbuf(c," datas over!\n",12);
            //memcpy(c->wbuf," datas over!\n",12);
            //c->wbytes = 12;
            //c->noreply = true;
             c->noreply = false;
            
            c->write_and_go = conn_closing;
           
        
        }
        else{
            
            //memcpy(c->wbuf,c->sdiff_story->element[c->ileft]->str,c->sdiff_story->element[c->ileft]->len);
            //c->wbytes = c->sdiff_story->element[c->ileft]->len; 
            Debug(" hget  %s  %s  ", c->sdiff_story->element[c->ileft]->str ,"data");
            redisReply *reply =  redisCommand(  c->thread->redis_conn, " hget  %s  %s  ", c->sdiff_story->element[c->ileft]->str ,"data");  
            if(reply->type == REDIS_REPLY_NIL  ){			// hgets  如果key，filed 不存在
    	        Debug(" REDIS_REPLY_NIL");
            }
            if(reply->type == REDIS_REPLY_STRING  ){			//   hgets  如果key，filed 存在有值
    	        Debug(" REDIS_REPLY_STRING");
    	            to_wbuf(c, reply->str,reply->len);
    	           Debug("hget  :[%s][%d]",reply->str,reply->len);
             }
             freeReplyObject(reply); 
            
        
        
        //接收客户端响应， 并验证返回报文
    	c->write_and_go = conn_waiting;
    	c->recv_flag = -2;
        
        }
    }
    
    
   
    return 0;
    
    
    
}

void check_client_reply(conn *c){
Debug(" check_client_reply  begin ............");
Debug("c->rbuf[%s][%d]",c->rbuf,c->rbytes);
    if( 0 == memcmp(c->rbuf,"ok\n",3)  ||  0 == memcmp(c->rbuf,"ok",2) ) {  
        // 成功，记录已发set, ileft +1 
        
        Debug(" client callback ok!...........................");
        char set_type_tmp[1024] = {};
        sprintf(set_type_tmp,"%s-%s",c->conn_client_info.types,c->conn_client_info.ip);
        redisReply *reply =  redisCommand(  c->thread->redis_conn, "sadd    %s   %s ",  set_type_tmp ,c->sdiff_story->element[c->ileft]->str);  
        redisCommandReplyCheckCallback(reply,REDIS_REPLY_INTEGER,sadd_reply,c , false);  
    }
    Debug(" client callback err!...........................");
       
}


    
    
  


