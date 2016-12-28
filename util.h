#ifndef _MIGRANT_UTIL_H
#define _MIGRANT_UTIL_H

struct data_check{
	char * data;
	int   data_len;
	char data_md5[32+1];
};
typedef struct data_check DATA;


char * from_key_find_value(char *data, const char *key);
int check_msg(char *msg,int msg_len,DATA* data);
char * from_msg_find_data(char *msg,char *dlme, int datalen);
int check_data_md5(char *data, char *md5);
char * _data_md5(char *data , int data_len);
 int create_multi_dir( char *path);

int  check_user_pw(conn *c);



#endif