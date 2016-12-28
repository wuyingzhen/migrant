/***********************************************************************            
*   ini 配置文件操作类函数
*   by wuyingzhen@hotmail.com

***********************************************************************/
#include <stdlib.h>
#include <string.h>
#include "readini.h"
#include "migrantlog.h"


#define ISSPACE(c)  ((c)==' '||(c)=='\n'||(c)=='\t')


static char **pContent = ( char ** ) NULL; /* 存放配置文件的内容 */

static int SplitFld(char *s1,int n,char *s2,int c);




/*去掉字符串中所有空白字符*/

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
 *	Function	:	去掉字符串右边连续的空白字符
 *	Input		:	char *s-----待处理的字符串
 *	Output		:	char *s-----处理后的字符串
 *	Return		:	处理后的字符串
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
* 函数名称: readINIT
* 功能描述: 按行读配置文件的内容到全局变量pContent中备用
* 入口参数:
*	       char		*fileName	配置文件名
* 出口参数: 无
* 返回值:   0  成功, －1  失败
************************************************************************/
int readINIT ( char *fileName )
{
	FILE 	*fp;              /* 文件指针 */
	char	line[TIPS_LINE_MAX+1]; /* 行数据 */
	int		i = 0;            /* 循环变量 */

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
* 函数名称: getINIT
* 功能描述: 根据配置文件中组名，项名，取得项值
* 入口参数:
*	       char		*grp	组名
*          char     *opt    项名
* 出口参数: 
*          char     *val    项值
* 返回值:   0  成功, －1  失败
************************************************************************/
int getINIT ( char *grp, char *opt, char *val ){
	int 	i = 0;  /* 循环变量 */
	int	grpMatch = false;  /* 组匹配标志 */
	int     optMatch = false;  /* 项匹配标志 */
	char	tmpStr[TIPS_LINE_MAX];  /* 临时变量 */

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
			else if( pContent[i][0] == '[' ) /* 说明已到下一grp,出错 */
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
* 函数名称: freeINIT
* 功能描述: 释放保存配置内容的内存空间
* 入口参数: 无
* 出口参数: 无
* 返回值:   无
************************************************************************/
void freeINIT ()
{
	int	i = 0; /* 循环变量 */
	while ( pContent[i] != (char *) NULL )
		free ( pContent[i++] );
	if ( pContent != ( char ** ) NULL )
		free ( pContent );
	pContent = ( char ** ) NULL;
}
/***********************************************************************
* 函数名称:  SplitFld
* 功能描述:	根据分隔符从母串中取子串
* 入口参数:	char  *s1  母串
*			int   n    要取的字段编号（从0开始编号）
*			int   c    分隔符
* 出口参数:	char  *s2  子串
* 返回值:	子串的长度
***********************************************************************/
static int SplitFld(char *s1,int n,char *s2,int c)
{
	int i;   /* 循环变量 */

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
* 函数名称: FilterCommen
* 功能描述: 过滤注释
* 入口参数:
*          char     *str    母串
* 出口参数: 无
* 返回值:   无
************************************************************************/
void FilterCommen ( char *str )
{
    char    *p;

    /* 以'#'到行末为注释 */
    p = ( char * ) strchr ( str, '#' );
    if ( p != NULL )
        p[0] = 0;
}

