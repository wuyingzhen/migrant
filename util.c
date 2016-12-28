#include "migrant.h"
#include "util.h"
#include "migrantlog.h"
#include "readini.h"
#include <string.h>
#include  <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <openssl/md5.h>


char * from_key_find_value(char *data,const char *key){
	if(!data || !key) return NULL;
	char *tmp_key = (char*) calloc(1,strlen(key)+2);
	memcpy(tmp_key,key,strlen(key));
	strncat(tmp_key,"=",1);

	char* pos = strstr(data,tmp_key);
	if(!pos ){
Debug(" not find key[%s] ",tmp_key);
	return NULL;
	}
	
	char *pos1 = strstr(pos,",");
	if(pos1 == NULL){
		pos1 = strstr(pos,";");
		if(pos1 == NULL) {
			Debug("not find key[%s]  value");
			return NULL;
		}
	}
	int value_len = pos1-(pos+strlen(key)) -1;
	char *value = (char*) calloc(1,value_len+1);
	memcpy(value,pos+strlen(key)+1,value_len);
Debug("from stream find key[%s]=value[%s]",key,value);
	
	free(tmp_key);
	return value;
}

 int check_msg(char *msg,int msg_len,DATA* data){
	char *value =NULL;
	value =  from_key_find_value(msg,"MD5"); 
//Debug(" recv data's MD5[%s]",value);
	if(value){
		memset(data->data_md5,0x00,32+1);
		memcpy(data->data_md5,value,32);
		free(value);
	}
	else
		return -1;
		
	value = from_key_find_value(msg,"length"); 
	if(value){
		data->data_len = atoi(value);  //  有问题，数据过大
		free(value);
	}
	else
		return -1;
	
	value = from_msg_find_data( msg,";",data->data_len );
Debug(" recv datas[%s][%d]",value,data->data_len);
	if(value){
		if( check_data_md5(value,data->data_md5) == -1)  
			return -1;
		else{
			
			data->data = value;
	//		Debug("from_msg_find_data,data=[%s] length=[%d]",data->data,data->data_len);
		}
	}
	else 
		return -1;	
}

 char * from_msg_find_data(char *msg,char *dlme, int datalen){
	if(!msg || !dlme) return NULL;
	
	char* pos = strstr(msg,dlme);
	if(!pos ){
Debug(" not find dlme[%s] ",dlme);
	return NULL;
	}
	
	char *value = (char*) calloc(1,datalen+1);
	memcpy(value,pos+strlen(dlme),datalen);
	return value;
}

 int check_data_md5(char *data, char *md5){
	char* datamd5 = _data_md5(data,strlen(data));
Debug(" recved data's MD5[%s]",md5);
Debug(" check  data's MD5[%s]",datamd5);
	if( memcmp(datamd5,md5,32) == 0){
Debug("  data's MD5 ok");
		free(datamd5);
		return 0;
	}
	else{
Debug("  data's MD5 err");
		free(datamd5);
		return -1;
	}

 	return 0;
}

 char * _data_md5(char *data , int data_len){
	if(!data) return NULL;
	char * m = (char*)calloc(1,33);
	unsigned  char  md[16];
    MD5((unsigned  char*)data, data_len,md);
	int i;
	for  (i = 0; i < 16; i++){
		sprintf(m,"%s%2.2x" ,m,md[i]);
	}
//Debug("md5=[%s]",m);
	return m;	
}

 int create_multi_dir( char *path) {
	int i, len;
	len = strlen(path);
	char dir_path[len+1];
	dir_path[len] = '\0';
	
	strncpy(dir_path, path, len);
	
	for (i=0; i<len; i++)
	{
	        if (dir_path[i] == '/' && i > 0)
	        {
	                dir_path[i]='\0';
	                if (access(dir_path, F_OK) < 0)
	                {
	                        if (mkdir(dir_path, 0755) < 0)
	                        {
	                                Debug("mkdir=%s:msg=%s", dir_path, strerror(errno));
	                                return -1;
	                        }
	                }
	                dir_path[i]='/';
	        }
	}
	
	return 0;
}

/*****************************与 client_info 相关的函数 ********************************/
/*
int client_info_array_init(client_info** client_info_array){
Debug("client_info_array_init   begin ...............................................");
    int ret;
    char title[128+1] = {}; 
    char num_str[64+1] = {};
    char tmp[192+1] = {};
    if( -1 == getINIT( "issue_ctr","title",title) ) return  -1; 
    if( -1 == getINIT( "issue_ctr","num",num_str) ) return  -1;
//Debug("title=[%s]",title);
//Debug("num_str=[%s]",num_str);
        
    int num = atoi(num_str);
//Debug("num=[%d]",num);
    client_info_array =( client_info**) malloc( (num+1) *sizeof(client_info*));
    
    if(client_info_array == NULL){
           Debug("client_info_array calloc error !");
           return -1;
    }
   // memset(client_info_array,0x00,num*sizeof(client_info));

    int i;
    for(i = 0;i < num; ++i){
        memset(tmp,0x00,sizeof(tmp));
        sprintf(tmp,"%s%d",title,i);
        client_info * tmp_c = calloc(1,sizeof(client_info));
        
//Debug("tmphhhh=[%s]",tmp);
//client_info_array[i]->ip = calloc(1,sizeof());
 	
       if( -1 ==   getINIT( tmp,"ip",tmp_c->ip) )return -1;
   	
        tmp_c->port = 0;
        if( -1 ==   getINIT( tmp,"user",tmp_c->user) )return -1;
        if( -1 ==   getINIT( tmp,"passwd",tmp_c->pw) )return -1;
        
    //    char  type[1024+1]={};
        tmp_c->types = calloc(1,1024);
        if( -1 ==   getINIT( tmp,"types",tmp_c->types) )return -1;
        tmp_c->total = 1000000; 
        	
       // ret = parse_type(type,client_info_array[i]->type);
 	client_info_array[i] = tmp_c;       
    }
//    client_info_array[i] = NULL;
   
    
               
}
*/

int client_info_array_init(client_info*** client_info_array_addr){
Debug(".......... client_info_array_init begin ..........");
    int ret;
    char title[128+1] = {}; 
    char num_str[64+1] = {};
    char tmp[192+1] = {};
    if( -1 == getINIT( "issue_ctr","title",title) ) return  -1; 
    if( -1 == getINIT( "issue_ctr","num",num_str) ) return  -1;
//Debug("title=[%s]",title);
//Debug("num_str=[%s]",num_str);
        
    int num = atoi(num_str);
//Debug("num=[%d]",num);
     *client_info_array_addr =( client_info**) calloc( (num+1) ,sizeof(client_info*));
    client_info** client_info_array = *client_info_array_addr;
    if(client_info_array == NULL){
           Debug("client_info_array calloc error !");
           return -1;
    }
   // memset(client_info_array,0x00,num*sizeof(client_info));

    int i;
    for(i = 0;i < num; ++i){
        memset(tmp,0x00,sizeof(tmp));
        sprintf(tmp,"%s%d",title,i);
        client_info * tmp_c = calloc(1,sizeof(client_info));
        
//Debug("tmphhhh=[%s]",tmp);
//client_info_array[i]->ip = calloc(1,sizeof());
 	
       if( -1 ==   getINIT( tmp,"ip",tmp_c->ip) )return -1;
        tmp_c->port = getvalue(tmp,"port");
        if( -1 ==   getINIT( tmp,"user",tmp_c->user) )return -1;
        if( -1 ==   getINIT( tmp,"passwd",tmp_c->pw) )return -1;
        tmp_c->types = calloc(1,1024);
        if( -1 ==   getINIT( tmp,"types",tmp_c->types) )return -1;
       // ret = parse_type(type,client_info_array[i]->type);
       
       tmp_c->total = getvalue(tmp,"total");
       
       
       client_info_array[i] = tmp_c;

    }
    client_info_array[num] = NULL;
               
}

void client_info_array_print(client_info** client_info_array){
Debug(".......... client_info_array_print begin ..........");
        int i = 0;
    for( ; client_info_array[i] != NULL;++i){
Debug("...........................................................");
Debug(" client_info_array[%d]->ip=[%s]",i,client_info_array[i]->ip); 
Debug(" client_info_array[%d]->port=[%d]",i,client_info_array[i]->port);
Debug(" client_info_array[%d]->user=[%s]",i,client_info_array[i]->user);
Debug(" client_info_array[%d]->pw=[%s]",i,client_info_array[i]->pw);
Debug(" client_info_array[%d]->types=[%s]",i,client_info_array[i]->types);
Debug(" client_info_array[%d]->total=[%d]",i,client_info_array[i]->total);   
              
    }      
}



int client_info_free(client_info* ci){
	if(ci->types != NULL)    
		free(ci->types);
	free(ci);
}

int client_info_array_free(client_info** client_info_array){
	if(client_info_array != NULL){
		int i = 0;
		while( client_info_array[i] != NULL){
			client_info_free(client_info_array[i]);
			++i;
		}	
	}
}

int client_info_array_search(char * ip){
Debug(".......... client_info_array_search begin ..........");
//Debug("ip =[%s]",ip);

   // assert(settings.client_info_array  != NULL);
    assert(ip != NULL);
    int i;
//    Debug("settings.client_info_array[i]->ip =[%s]",settings.client_info_array[0]->ip);
    for(i = 0; settings.client_info_array[i] != NULL; ++i){
//        Debug("settings.client_info_array[i]->ip =[%s]",settings.client_info_array[i]->ip);
        if(   0 == memcmp(   settings.client_info_array[i]->ip,ip,strlen(ip)) )
            return i;
        
    }
    return -1;
    
}

int  check_user_pw(conn *c){
Debug(".......... check_user_pw begin ..........");
//Debug(" check_user_pw begin .............");
//Debug(" c->rbuf[%s][%d]",c->rbuf,c->rbytes);
    int ret = client_info_array_search(c->conn_client_info.ip);
    if(ret == -1){  // 找不到对应ip，失败
        memcpy(c->wbuf,"This ip is not login!\n",22);
        c->wbytes = 22;
        return -1;   
    }
    char tmp[1024+1] = {};
    char tmp1[1024+1] = {};
//    sprintf(tmp,"%s:%s",c->conn_client_info.user,c->conn_client_info.pw);
//Debug(" client user:passwd[%s]",tmp);
    sprintf(tmp,"%s:%s\n",settings.client_info_array[ret]->user,settings.client_info_array[ret]->pw);
    sprintf(tmp1,"%s:%s",settings.client_info_array[ret]->user,settings.client_info_array[ret]->pw);
Debug(" server save user:passwd[%s]",tmp1);
    
    if( 0 != memcmp(c->rbuf,tmp1,c->rbytes)   &&     0 != memcmp(c->rbuf,tmp,c->rbytes)       ){
        memcpy(c->wbuf,"user or passwd is wrong!\n",25);
        c->wbytes = 25;
        return -1;       
    }
    c->conn_client_info.total = settings.client_info_array[ret]->total;
    memcpy(c->conn_client_info.user,settings.client_info_array[ret]->user,sizeof(c->conn_client_info.user));
    memcpy(c->conn_client_info.pw,settings.client_info_array[ret]->pw,sizeof(c->conn_client_info.pw));
    c->conn_client_info.types = calloc(1024,1);
    memcpy(c->conn_client_info.types,settings.client_info_array[ret]->types,1023);
    
    to_wbuf(c,"check ip ok! will send data.\n",29);

   return 0;
}


void check_total(int fd, short events, void *ctx){   
Debug("check_total  begin ..##############################................"); 
    conn *c = (conn*)ctx;
    Debug("send data[%d], set data total[%d]",c->send_total_bytes,c->conn_client_info.total);
    Debug("recv data[%d], set data total[%d]",c->recv_total_bytes,c->conn_client_info.total);
    
#ifdef  _MIGRANT_DISTRIBUTE_MODE_    
    if( c->send_total_bytes >= c->conn_client_info.total)
    {
        Debug(" send data >= total  so close fd");  
        c->state = conn_closing; 
    }
    else{
        Debug("send data < total  so  continue");
        
    }
    
#else
     if( c->recv_total_bytes >= c->conn_client_info.total)
    {
        Debug(" recv data >= total  so close fd");  
        c->state = conn_closing; 
    }
    else{
        Debug("recv data < total  so  continue, ");
        c->recv_total_bytes = 0;
        
    }


#endif 
    
    
}
















































	

	
		
	
	
	
	 	
	
