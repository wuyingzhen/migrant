#include "myhiredis.h"
#include "migrantlog.h"


void redisCommandReplyCheckCallback(  redisReply *reply,  int type , redisReplyCallbackFn  func , void* arg, bool nofree){
     if( reply->type == type){
        func(reply,arg);
        if( nofree == true)  return ;
        else 
            freeReplyObject(reply);
     }
     else
        redisCommandReplyCheck(reply);	
}

  /*
    switch(reply->type){
        case REDIS_REPLY_STATUS:
            Debug(" REDIS_REPLY_STATUS");
            if( reply->type == type) 
                 func(reply,arg);
            else
     	        Debug("redisCommand reply:[%s][%d]",reply->str,reply->len);
            
            break;
       case REDIS_REPLY_ERROR:
            Debug(" REDIS_REPLY_ERROR");
            if( reply->type == type) 
                 func(reply,arg);
            else
                Debug("redisCommand reply:[%s][%d]",reply->str,reply->len);
        
        
            break;
        case  REDIS_REPLY_INTEGER:
            
            break;
        case REDIS_REPLY_NIL:
            
            break;
        case REDIS_REPLY_STRING:
            
            break;
        case REDIS_REPLY_ARRAY:
            
            break;
        
    
    if(reply->type == REDIS_REPLY_STATUS  ){                           // set ����
     	Debug(" REDIS_REPLY_STATUS");
     	Debug("redisCommand reply:[%s][%d]",reply->str,reply->len);
    }
    if(reply->type == REDIS_REPLY_ERROR  ){                        // hset key ���Ͳ��ԣ�����
     	Debug(" REDIS_REPLY_ERROR");
     	Debug("redisCommand reply:[%s][%d]",reply->str,reply->len);
    }
    if(reply->type == REDIS_REPLY_INTEGER  ){                   // hset ����  �ɹ�����1 ;  hexists ���� ��0 �����ڣ�1 ����
    	Debug(" REDIS_REPLY_INTEGER");
    	long ret = reply->integer;
    	Debug("redisCommand reply: [%ld]",ret);
    }
     if(reply->type == REDIS_REPLY_NIL  ){			// hgets  ���key��filed ������
    	Debug(" REDIS_REPLY_NIL");
    }
     if(reply->type == REDIS_REPLY_STRING  ){			// get ����  ;  hgets  ���key��filed ������ֵ
    	Debug(" REDIS_REPLY_STRING");
    	char *data = reply->str;
    	Debug("redisCommand reply:[%s][%d]",data,reply->len);
    }
    if(reply->type == REDIS_REPLY_ARRAY  ){                        // hkeys  hvals ���أ����key�����ڣ�reply->elements ����0
    	Debug(" REDIS_REPLY_ARRAY");
    	Debug("redisCommand reply:----------");
    	Debug("elements:[%d]",reply->elements);
    	Debug("-----------------------------------");
    	int i;
    	for(i=0;i <reply->elements ; ++i){
    		Debug("[%d]---[%s][%d]",i,reply->element[i]->str,reply->element[i]->len);
        }
    }
    		 
    freeReplyObject(reply);    // �첽�ص�����ִ���Ժ���Զ���reply �ͷ�
*/


void redisCommandReplyCheck(  redisReply *reply ){
    if(reply->type == REDIS_REPLY_STATUS  ){                           // set ����
     	Debug(" REDIS_REPLY_STATUS");
     	Debug("redisCommand reply:[%s][%d]",reply->str,reply->len);
    }
    if(reply->type == REDIS_REPLY_ERROR  ){                        // hset key ���Ͳ��ԣ�����
     	Debug(" REDIS_REPLY_ERROR");
     	Debug("redisCommand reply:[%s][%d]",reply->str,reply->len);
    }
    if(reply->type == REDIS_REPLY_INTEGER  ){                   // hset ����  �ɹ�����1 ;  hexists ���� ��0 �����ڣ�1 ����
    	Debug(" REDIS_REPLY_INTEGER");
    	long ret = reply->integer;
    	Debug("redisCommand reply: [%ld]",ret);
    }
     if(reply->type == REDIS_REPLY_NIL  ){			// hgets  ���key��filed ������
    	Debug(" REDIS_REPLY_NIL");
    }
     if(reply->type == REDIS_REPLY_STRING  ){			// get ����  ;  hgets  ���key��filed ������ֵ
    	Debug(" REDIS_REPLY_STRING");
    	char *data = reply->str;
    	Debug("redisCommand reply:[%s][%d]",data,reply->len);
    }
    if(reply->type == REDIS_REPLY_ARRAY  ){                        // hkeys  hvals ���أ����key�����ڣ�reply->elements ����0
    	Debug(" REDIS_REPLY_ARRAY");
    	Debug("redisCommand reply:----------");
    	Debug("elements:[%d]",reply->elements);
    	Debug("-----------------------------------");
    	int i;
    	for(i=0;i <reply->elements ; ++i){
    		Debug("[%d]---[%s][%d]",i,reply->element[i]->str,reply->element[i]->len);
        }
    }
    		 
    freeReplyObject(reply);    // �첽�ص�����ִ���Ժ���Զ���reply �ͷ�
	
}





/*
static void redis_disconnect(redisAsyncContext *c, void *r, void *privdata){
      // ����Ҫ��ʱ
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    event_add(client->write_timer, &timeout);
    redisAsyncDisconnect(c);	
	
}
*/


/*
static void hget_callback(redisAsyncContext *c, void *r, void *privdata) {
    redisReply *reply = r;
    if (reply == NULL) return;
 
    pigeon *client = (pigeon*) privdata; 
    if(client == NULL) return ;	
  	
    redisCommandReplyCheck(reply);
    
  //  redis_disconnect(c,client);
  
}
*/
/*
static void connect_callback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        Debug("Error: %s", c->errstr);
        return;
    }
    Debug("Connected...");
}

static void disconnect_callback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        Debug("Error: %s", c->errstr);
        return;
    }
    Debug("Disconnected...");
   
    
}

*/

/*
static int redis_search(redisAsyncContext *conn, pigeon* client){
Debug(" redis_search begin ...........");
    redisAsyncCommand(conn, hget_callback, client, "hget  B a");	
	
}
*/

/*
int redis_connect(redisAsyncContext *conn, struct event_base *base,char *ip , int port ){
Debug(" redis_connect begin ....");
    // ��������redis
    conn = redisAsyncConnect(ip, port);
    if (conn->err) {
        // Let *c leak for now... 
        Debug("Error: %s", conn->errstr);
        return -1;
    }
    
    redisLibeventAttach(conn,base);
    
    // ���ûص�����
    redisAsyncSetConnectCallback(conn,connect_callback);
    redisAsyncSetDisconnectCallback(conn,disconnect_callback);
    
    return 0;
     
	   
}

*/



