/***********************************************************************            
*   ini �����ļ������ຯ��
*   by wuyingzhen@hotmail.com

***********************************************************************/
#include <stdlib.h>
#include <string.h>
#include "readini.h"
#include "migrantlog.h"


#define ISSPACE(c)  ((c)==' '||(c)=='\n'||(c)=='\t')


static char **pContent = ( char ** ) NULL; /* ��������ļ������� */

static int SplitFld(char *s1,int n,char *s2,int c);




/*ȥ���ַ��������пհ��ַ�*/

static void AllTrim(char *ptr)
{
    if( 0 == ptr ) return;
	char	*p1;
	char	*p2;
	int		i;
	p1=p2=ptr;
	i=0;
	do{
		if(ISSPACE(p1[0])){
			p1++;
		}
		else{
			p2[i]=p1[0];
			i++;
			p1++;
		}
	}while(p1[0]);
	p2[i]=0;
}


/*
 *	Name		:	RTrim
 *	Function	:	ȥ���ַ����ұ������Ŀհ��ַ�
 *	Input		:	char *s-----��������ַ���
 *	Output		:	char *s-----�������ַ���
 *	Return		:	�������ַ���
 */
static char *
RTrim( char *s )
{
	char *p;

	if( NULL == s ) return s;
	for(p=(char *)strchr(s,'\0'); p>s && ISSPACE(*(p-1)); p--)
		;
	*p = '\0';
	return s;
}


/***********************************************************************
* ��������: readINIT
* ��������: ���ж������ļ������ݵ�ȫ�ֱ���pContent�б���
* ��ڲ���:
*	       char		*fileName	�����ļ���
* ���ڲ���: ��
* ����ֵ:   0  �ɹ�, ��1  ʧ��
************************************************************************/
int readINIT ( char *fileName )
{
	FILE 	*fp;              /* �ļ�ָ�� */
	char	line[TIPS_LINE_MAX+1]; /* ������ */
	int		i = 0;            /* ѭ������ */

	if ( (fp = fopen(fileName, "r")) == NULL )
	{
		fprintf(stderr,"open file[%s] error:[%s]",fileName ,strerror(errno));
		return -1;
	}

	while ( fgets(line, TIPS_LINE_MAX, fp) != NULL )
	{
		/* delete '\n' */
		line [ strlen(line) - 1 ] = 0;

		/* filter commentary */
		FilterCommen ( line );

		/* delete left and right space */
		AllTrim( line );

		/* delete all space */
		AllTrim( line );
		if ( strlen(line) == 0 )
			continue;

		/* allocate memory */
		if ( pContent == (char **)NULL )
			pContent = ( char ** ) malloc ( sizeof(char *) * 2 );
		else
			pContent = ( char ** ) realloc ( pContent, sizeof(char *) * (i + 2) );	
		if ( pContent == ( char ** )NULL )
		{
			Debug("malloc error:[%s]",strerror(errno));
			fclose ( fp );
			return -1;
		}

		pContent[i] = ( char * ) malloc ( strlen(line) + 1 );
		if ( pContent[i] == (char *)NULL )
		{
			Debug("malloc error:[%s]",strerror(errno));
			fclose ( fp );
			freeINIT ();
			return -1;
		}
		strcpy ( pContent[i], line );
		pContent[++i] = ( char * ) NULL;
	}

	fclose ( fp );
	return 0;
}
/***********************************************************************
* ��������: getINIT
* ��������: ���������ļ���������������ȡ����ֵ
* ��ڲ���:
*	       char		*grp	����
*          char     *opt    ����
* ���ڲ���: 
*          char     *val    ��ֵ
* ����ֵ:   0  �ɹ�, ��1  ʧ��
************************************************************************/
int getINIT ( char *grp, char *opt, char *val ){
	int 	i = 0;  /* ѭ������ */
	int	grpMatch = false;  /* ��ƥ���־ */
	int     optMatch = false;  /* ��ƥ���־ */
	char	tmpStr[TIPS_LINE_MAX];  /* ��ʱ���� */

	if ( pContent[0] == ( char * )NULL )
	{
		Debug("Memory not initialized:[%s]",opt);
		return -1;
	}

	sprintf ( tmpStr, "[%s]", grp );
	while ( pContent[i] != (char *)NULL )
	{	//Debug("tmpStr=[%s]",tmpStr);
		//Debug("[%d]-[%s]",i,	pContent[i]);	
		if ( grpMatch == true )
		{
			SplitFld(pContent[i],0,tmpStr,'=');			
			if ( strcmp(opt,tmpStr) == 0 )
			{					
				strcpy ( tmpStr, (char *)strchr(pContent[i], '=') + 1 );			
				AllTrim( tmpStr );
				
				if( strlen( tmpStr ) != 0 )
				{
					strcpy ( val, tmpStr );
					optMatch = true;					
					return 0;
				}
				else
				{
					Debug(" option[%s] is null in grp[%s]", opt, grp);
					return -1;
				}
			}
			else if( pContent[i][0] == '[' ) /* ˵���ѵ���һgrp,���� */
			{
				Debug( "no such option[%s] in grp[%s]", opt, grp );
				return 0;
			}
		}		

		if ( strcmp(tmpStr, pContent[i]) == 0 )	
			grpMatch = true;

		i++;
	}

	if ( grpMatch == false )
	{
		Debug( "No such group[%s]", grp );
		return -1;
	}
	if( grpMatch == true )
	{
		Debug( "No such option[%s] in grp[%s]", opt, grp );
		return -1;
	}

	return 0;
}

int  getvalue(char *grp,char *opt){
	char tmp[TIPS_LINE_MAX];
	memset(tmp,0x00,TIPS_LINE_MAX);
	getINIT(grp,opt,tmp);
//	Debug("[%s]-[%s]=%d",grp,opt,atoi(tmp));
	return atoi(tmp);
}
/***********************************************************************
* ��������: freeINIT
* ��������: �ͷű����������ݵ��ڴ�ռ�
* ��ڲ���: ��
* ���ڲ���: ��
* ����ֵ:   ��
************************************************************************/
void freeINIT ()
{
	int	i = 0; /* ѭ������ */
	while ( pContent[i] != (char *) NULL )
		free ( pContent[i++] );
	if ( pContent != ( char ** ) NULL )
		free ( pContent );
	pContent = ( char ** ) NULL;
}
/***********************************************************************
* ��������:  SplitFld
* ��������:	���ݷָ�����ĸ����ȡ�Ӵ�
* ��ڲ���:	char  *s1  ĸ��
*			int   n    Ҫȡ���ֶα�ţ���0��ʼ��ţ�
*			int   c    �ָ���
* ���ڲ���:	char  *s2  �Ӵ�
* ����ֵ:	�Ӵ��ĳ���
***********************************************************************/
static int SplitFld(char *s1,int n,char *s2,int c)
{
	int i;   /* ѭ������ */

	for (i=0;i<n;i++,s1++) {
		while (*s1 && *s1!=c && *s1!='\n')
			s1++;
		if (*s1!=c)
			return *s2=0;
	}
	for (i=0;*s1 && *s1!=c && *s1!='\n';i++,s1++,s2++)
		*s2=*s1;
	*s2=0;
	return i;
}

/***********************************************************************
* ��������: FilterCommen
* ��������: ����ע��
* ��ڲ���:
*          char     *str    ĸ��
* ���ڲ���: ��
* ����ֵ:   ��
************************************************************************/
void FilterCommen ( char *str )
{
    char    *p;

    /* ��'#'����ĩΪע�� */
    p = ( char * ) strchr ( str, '#' );
    if ( p != NULL )
        p[0] = 0;
}

