/*
 *  migrant - 候鸟
 *  高并发的多线程非阻塞网络io框架
 *       
 * by wuyingzhen@hotmail.com
 *  
 *
 */
 
#ifndef __MIGRANT_H__
#define __MIGRANT_H__
 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <event.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>


//#define _MIGRANT_DISTRIBUTE_MODE_


// redis 模式
#define _MIGRANT_USE_REDIS
#ifdef _MIGRANT_USE_REDIS
#include "myhiredis.h"
#endif


// lua config 配置文件模式
//#define _MIGRANT_LUA_CONFIG
#ifdef _MIGRANT_LUA_CONFIG
#include "lua_conf.h" 
#endif


// ini 格式config 配置文件模式
#define _MIGRANT_INI_CONFIG
#ifdef    _MIGRANT_INI_CONFIG
#include "readini.h"
#endif




#define IS_UDP(x) (x == udp_transport)

/** Maximum length of a key. */
#define KEY_MAX_LENGTH 250

/** Size of an incr buf. */
#define INCR_MAX_STORAGE_LEN 24

#define DATA_BUFFER_SIZE 2048
#define UDP_READ_BUFFER_SIZE 65536
#define UDP_MAX_PAYLOAD_SIZE 1400
#define UDP_HEADER_SIZE 8
#define MAX_SENDBUF_SIZE (256 * 1024 * 1024)
/* I'm told the max length of a 64-bit num converted to string is 20 bytes.
 * Plus a few for spaces, \r\n, \0 */
#define SUFFIX_SIZE 24

/** Initial size of list of items being returned by "get". */
#define ITEM_LIST_INITIAL 200

/** Initial size of list of CAS suffixes appended to "gets" lines. */
#define SUFFIX_LIST_INITIAL 20

/** Initial size of the sendmsg() scatter/gather array. */
#define IOV_LIST_INITIAL 400

/** Initial number of sendmsg() argument structures to allocate. */
#define MSG_LIST_INITIAL 10

/** High water marks for buffer shrinking */
#define READ_BUFFER_HIGHWAT 8192
#define ITEM_LIST_HIGHWAT 400
#define IOV_LIST_HIGHWAT 600
#define MSG_LIST_HIGHWAT 100

/* Binary protocol stuff */
#define MIN_BIN_PKT_LENGTH 16



/*
 *  函数声明
 */



void STATS_LOCK();
void STATS_UNLOCK();



typedef struct {
    char ip[15+1];
    unsigned int port;
    
    char user[64+1];    
    char pw[32+1];
    
    //char **types;
    char *types;
    
    unsigned int total;
    
    //void **stream_type;
}client_info;



enum try_read_result {
    READ_DATA_RECEIVED,
    READ_NO_DATA_RECEIVED,
    READ_ERROR,            // an error occured (on the socket) (or client closed connection) 
    READ_MEMORY_ERROR     // failed to allocate more memory  
};

enum try_write_result {
    WRITE_DATA_OK,
    WRITE_ERROR,            // an error occured (on the socket) (or client closed connection) 
    WRITE_MEMORY_ERROR     // failed to allocate more memory  
};



/**
 * 连接所处的状态
 */
enum conn_states {
    conn_listening,  /**< the socket which listens for connections */
    conn_new_cmd,    /**< Prepare connection for next command */
    conn_waiting,    /**< waiting for a readable socket */
    conn_read,       /**< reading in a command line */
    conn_parse_cmd,  /**< try to parse a command from the input buffer */
    conn_write,      /**< writing out a simple response */
    conn_nread,      /**< reading in a fixed number of bytes */
    conn_swallow,    /**< swallowing unnecessary bytes w/o storing */
    conn_closing,    /**< closing this connection */
    conn_mwrite,     /**< writing out many items sequentially */
    conn_max_state   /**< Max state value (used for assertion) */
};

enum bin_substates {
    bin_no_state,
    bin_reading_set_header,
    bin_reading_cas_header,
    bin_read_set_value,
    bin_reading_get_key,
    bin_reading_stat,
    bin_reading_del_header,
    bin_reading_incr_header,
    bin_read_flush_exptime,
    bin_reading_sasl_auth,
    bin_reading_sasl_auth_data,
    bin_reading_touch_key,
};

enum protocol {
    ascii_prot = 3, /* arbitrary value. */
    binary_prot,
    negotiating_prot /* Discovering the protocol */
};

enum network_transport {
    local_transport, /* Unix sockets*/
    tcp_transport,
    udp_transport
};

enum item_lock_types {
    ITEM_LOCK_GRANULAR = 0,
    ITEM_LOCK_GLOBAL
};

enum store_item_type {
    NOT_STORED=0, STORED, EXISTS, NOT_FOUND
};

/**
 * Stats stored per-thread.
 */
struct thread_stats {
    pthread_mutex_t   mutex;
    uint64_t          get_cmds;
    uint64_t          get_misses;
    uint64_t          touch_cmds;
    uint64_t          touch_misses;
    uint64_t          delete_misses;
    uint64_t          incr_misses;
    uint64_t          decr_misses;
    uint64_t          cas_misses;
    uint64_t          bytes_read;
    uint64_t          bytes_written;
    uint64_t          flush_cmds;
    uint64_t          conn_yields; /* # of yields for connections (-R option)*/
    uint64_t          auth_cmds;
    uint64_t          auth_errors;
    //struct slab_stats slab_stats[MAX_NUMBER_OF_SLAB_CLASSES];
};

/**
 * Global stats.
 */
struct stats {
    pthread_mutex_t mutex;
    unsigned int  curr_items;
    unsigned int  total_items;
    uint64_t      curr_bytes;
    unsigned int  curr_conns;
    unsigned int  total_conns;
    uint64_t      rejected_conns;
    unsigned int  reserved_fds;
    unsigned int  conn_structs;
    uint64_t      get_cmds;
    uint64_t      set_cmds;
    uint64_t      touch_cmds;
    uint64_t      get_hits;
    uint64_t      get_misses;
    uint64_t      touch_hits;
    uint64_t      touch_misses;
    uint64_t      evictions;
    uint64_t      reclaimed;
    time_t        started;          /* when the process was started */
    bool          accepting_conns;  /* whether we are currently accepting */
    uint64_t      listen_disabled_num;
    unsigned int  hash_power_level; /* Better hope it's not over 9000 */
    uint64_t      hash_bytes;       /* size used for hash tables */
    bool          hash_is_expanding; /* If the hash table is being expanded */
    uint64_t      expired_unfetched; /* items reclaimed but never touched */
    uint64_t      evicted_unfetched; /* items evicted but never touched */
    bool          slab_reassign_running; /* slab reassign in progress */
    uint64_t      slabs_moved;       /* times slabs were moved around */
};

struct settings {
    size_t maxbytes;
    int maxconns;
    int port;                  								// TCP 端口
    char *inter;          								   // ip  点分十进制
    char *socketpath;   						       // 本地套接字
    int num_threads;        						  // 工作线程数
    enum protocol binding_protocol;
    int backlog;                                         // listen 监听的数量
    bool maxconns_fast;     /* Whether or not to early close connections */
	//  int udpport;       // UDP 端口
	// int verbose;
     
    client_info** client_info_array;         // 客户信息数组
};


extern struct stats stats;
extern time_t process_started;
extern struct settings settings;


// 多个线程, 每个线程一个 event_base
typedef struct {
    pthread_t thread_id;                                                         		// 线程id
    struct event_base *base;                                                   		// 线程的event_base 
    struct event notify_event;                                                              // event 结构体, 用于管道读写事件的监听
    
    int notify_receive_fd;                                                        		// 读写管道文件描述符 - 读端
    int notify_send_fd;                                                                         // 读写管道文件描述符 - 写端
    
    struct thread_stats stats;                                                                 // 线程的状态
    struct conn_queue *new_conn_queue;                                            // 这个线程需要处理的连接队列
    
#ifdef _MIGRANT_USE_REDIS
	redisContext *redis_conn;                                                            // 与redis的同步链接 
	//redisAsyncContext *redis_conn;
#endif

    
} LIBEVENT_THREAD;

typedef struct {
    pthread_t thread_id;        
    struct event_base *base;
} LIBEVENT_DISPATCHER_THREAD;

/**
 * 每个连接封装到一个conn结构体中
 */
typedef struct conn conn;
struct conn {
    int    sfd;                                        // sockfd
    enum conn_states  state;              // 连接状态
    bool   is_allowed;                          // 此连接是否经过服务器验证    // 不用此标志，换做recv_flag
    bool   is_client_close;                    // 服务器是否主动断开连接  
    
    client_info  conn_client_info;         // 客户端信息
    
    enum bin_substates substate;
    struct event event;
    short  ev_flags;
    short  which;                               // 刚刚触发的事件
    short   recv_flag;                  //  接收处理标志，如果接受处理成功0，失败-1 ；    
    
    char   *rbuf;                        // 接收报文指针
    char   *rcurr;                                                                                 // 已经解析了一部分的命令, 指向已经解析结束的地方,暂时未用到
   unsigned  int    rsize;                                                                                    // rbuf 已分配的大小
   unsigned  int    rbytes;                                                                                  // 已经接受到数据大小
    char *databegin;
    unsigned int datalen;
    
    char   *wbuf;                                                                                // buffer to write
    char   *wcurr;                                                                               // 指向已经返回的地方
   unsigned int    wsize;                                                                                  // 写大小
   unsigned int    wbytes;                                                                                // 写的数据大小
    
    enum conn_states  write_and_go;                                                 // 当写回结束后需要即刻转变的状态
    void   *write_and_free;                                                                  // free this memory after finishing writing 
    char   *ritem;                                                                                //   when we read in an item's value, it goes here 
    void   *item;                                                                                 // 指向当下需要完成的任务
    unsigned int    send_total_bytes;                                                                                  // 用来接收字节数
    unsigned int    recv_total_bytes;                                                                                  // 用来接收字节数
    int    ileft;                                                                                      // 记录任务数量 ,
    enum protocol protocol;                                                               //  which protocol this connection speaks 
    enum network_transport transport;                                              //  what transport is used by this connection 
    bool   noreply;                                                                              //   是否回复报文，If true 不回复 ，会复位，每次不要回复都重新设置
    conn   *next;                                                                                 // conn结构体的一个链表
    LIBEVENT_THREAD *thread;                                                          // 指向服务于此连接的线程
    
    struct event check_total_timer;                                                   // 检查每个端口接收发送流量的定时器
    
#ifdef _MIGRANT_USE_REDIS
    redisReply *sdiff_story;                                                              // 用来保存sdiff 结果
#endif
};


/*
 *  函数声明
 */


conn * conn_new(const int sfd, enum conn_states init_state,
		const int event_flags,const int read_buffer_size,
		 enum network_transport transport,struct event_base *base);


#define ITEMS_PER_ALLOC 64

/* An item in the connection queue. */
typedef struct conn_queue_item CQ_ITEM;
struct conn_queue_item {
    int               sfd;
    enum conn_states  init_state;
    int               event_flags;
    int               read_buffer_size;
    enum network_transport     transport;
    CQ_ITEM          *next;
};

/* A connection queue. */
typedef struct conn_queue CQ;
struct conn_queue {
    CQ_ITEM *head;
    CQ_ITEM *tail;
    pthread_mutex_t lock;
    pthread_cond_t  cond;
};


/*  key-value struct */
typedef struct key_value KV;
struct key_value{
	char * key;
	char * value;
};





#endif