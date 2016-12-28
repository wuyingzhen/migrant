/******************************************************

*   文件系统缓存
* by   wuyingzhen@hotmial.com


******************************************************/

#include "migrant.h"
#include "util.h"
#include "migrantlog.h"
#include <string.h>
#include  <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>


char *term_key[] = {
	"Sender",		\
	"OTime",		\
	"InTime",		\
	"priority"
};

#define TERM_KEY_LEN 4
#define M_FREE(c)  if(!c) free(c)
/*
 *  函数声明
 */
 static void file_insert(FILE* fd,char *key,char* value);
 static  FILE*  file_init(char * filename);
 static void file_close(FILE* fd);
 static void file_write_kv(FILE* fd,char *key,char* value);


/*
 *  对外的函数 ，对接收到的数据进行处理
 */
/*
int save_data(conn *c){

//	Debug("c->rbuf[%s]",c->rbuf);
	if( c->rbuf == NULL || c->rbytes == 0) return -1;
		
	// check msg : 1: msg_len  2:md5
	DATA msg_data;
Debug("check data   校对数据md5");	
	int ret = check_msg(c->rbuf,c->rbytes,&msg_data);
	if( ret == -1){
		Debug("check msg error.");
		return -1;	
	}
	
	// 建立目录和文件	
	char *value =  from_key_find_value(c->rbuf,"FileName");
	char *value1 =  from_key_find_value(c->rbuf,"TYPE"); 
	char *value2 =  from_key_find_value(c->rbuf,"OTime"); 
	if(!value || !value1 || !value2 ){
		//if(msg_data.data )
			//free(msg_data.data);
			M_FREE(msg_data.data);
			M_FREE(value);
			M_FREE(value1);
			M_FREE(value2);
		return -1;
	}
	
	char home[64+1] = {"/data/cts/wuyz/src/data"};
	char filepath[512+1];
	char filepathfile[1024+1];
	char arr_date[10+1];
	memset(arr_date,0x00,sizeof(arr_date));
	memset(filepath,0x00,sizeof(filepath));
	memset(filepathfile,0x00,sizeof(filepathfile));
	memcpy(arr_date,value2,10);
	sprintf(filepath,"%s/%s/%s/",home,value1,arr_date);
	sprintf(filepathfile,"%s%s",filepath,value);
	
	// 建立目录
	if( create_multi_dir(filepath) == -1)
		return -1;
	
	// create file
	FILE * fd = file_init(filepathfile);
	
	// insert data to file
//Debug("msg_data.data=[%s]",msg_data.data);
	file_insert(fd,"data",msg_data.data);
//	file_insert(fd,"md5",msg_data.data_md5);
//	file_insert(fd,"length",msg_data.data_len);
	file_insert(fd,"FileName",value);
	free(msg_data.data);
	free(value);
	int i;
	for(i = 0; i < TERM_KEY_LEN ; ++i){
		char *value = from_key_find_value(c->rbuf,term_key[i]); 
		file_insert(fd,term_key[i],value);
		free(value);
	}
	//file_insert(fd,"data",c->rbuf);
	
	// close file
	file_close(fd);
	
	return 0;
}
*/

int save_as_file(conn *c, DATA msg_data){
    	// 建立目录和文件	
	char *value =  from_key_find_value(c->rbuf,"FileName");
	char *value1 =  from_key_find_value(c->rbuf,"TYPE"); 
	char *value2 =  from_key_find_value(c->rbuf,"OTime"); 
	if(!value || !value1 || !value2 ){
		//if(msg_data.data )
			//free(msg_data.data);
			M_FREE(msg_data.data);
			M_FREE(value);
			M_FREE(value1);
			M_FREE(value2);
		return -1;
	}
	
	char home[64+1] = {"/data/cts/wuyz/src/data"};
	char filepath[512+1];
	char filepathfile[1024+1];
	char arr_date[10+1];
	memset(arr_date,0x00,sizeof(arr_date));
	memset(filepath,0x00,sizeof(filepath));
	memset(filepathfile,0x00,sizeof(filepathfile));
	memcpy(arr_date,value2,10);
	sprintf(filepath,"%s/%s/%s/",home,value1,arr_date);
	sprintf(filepathfile,"%s%s",filepath,value);
	
	// 建立目录
	if( create_multi_dir(filepath) == -1)
		return -1;
	
	// create file
	FILE * fd = file_init(filepathfile);
	
	// insert data to file
//Debug("msg_data.data=[%s]",msg_data.data);
	file_insert(fd,"data",msg_data.data);
//	file_insert(fd,"md5",msg_data.data_md5);
//	file_insert(fd,"length",msg_data.data_len);
	file_insert(fd,"FileName",value);
	free(msg_data.data);
	free(value);
	int i;
	for(i = 0; i < TERM_KEY_LEN ; ++i){
		char *value = from_key_find_value(c->rbuf,term_key[i]); 
		file_insert(fd,term_key[i],value);
		free(value);
	}
	//file_insert(fd,"data",c->rbuf);
	
	// close file
	file_close(fd);
	
	return 0;	
	
}





/* ****************************文件操作的辅助函数**************************************/
static FILE*  file_init(char * filename){
	if(!filename) return NULL;
	char pathfile[1024+1] = {};
//	sprintf(pathfile,"%s/%s","/data/cts/wuyz/src/data",filename);
Debug(" 打开文件open file[%s]", filename);
	FILE *fd = fopen(filename,"w+");
	if(!fd){
		Debug(" fopen error[%s].",filename);
		return NULL;
	}
	char title_begin[]={"QXdata{"};
	fwrite(title_begin,1,strlen(title_begin),fd);
	
	return fd;	
}

static void file_close(FILE* fd){
	if(!fd) return ;
	char title_end[] = {"\n}\n"};
	fwrite(title_end,1,strlen(title_end),fd);
	fclose(fd);		
}

static void file_insert(FILE* fd,char *key,char* value){
	if(!fd || !key)  return  ;
	fseek(fd,-1,SEEK_END);
	char ch = fgetc(fd);
	if( ch != '{')
		fputc(',',fd);
	file_write_kv(fd,key,value);	
}


static void file_write_kv(FILE* fd,char *key,char* value){
	//filefd:write( '\n\t' .. key ..' = "' .. val .. '"')
	fputc('\n',fd);
	fputc('\t',fd);
	fwrite(key,1,strlen(key),fd);
	fputc('=',fd);
	fputc('"',fd);
	fwrite(value,1,strlen(value),fd);
	fputc('"',fd);
}

