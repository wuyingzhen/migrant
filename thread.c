/*
 *  migrant - 候鸟
 *  高并发的多线程非阻塞网络io框架
 *       
 * by wuyingzhen@hotmail.com
 *  
 *
 */

#include "migrant.h"
#include "migrantlog.h"
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>



#define ITEMS_PER_ALLOC 64
#define ITEMS_PER_ALLOC 64

/*
 *  变量声明和初始化
 */
static LIBEVENT_THREAD *threads;       															// 线程结构LIBEVENT_THREAD数组 
static LIBEVENT_DISPATCHER_THREAD dispatcher_thread;                            //  主线程结构体


//static pthread_key_t item_lock_type_key;

static int init_count = 0;                                                                         	// 开启的线程个数
static pthread_mutex_t init_lock;                                                               // 线程个数锁
static pthread_cond_t init_cond;

/* Lock for cache operations (item_*, assoc_*) */
pthread_mutex_t cache_lock;

/* Lock for global stats */
static pthread_mutex_t stats_lock;

/* Free list of CQ_ITEM structs */
static CQ_ITEM *cqi_freelist;
static pthread_mutex_t cqi_freelist_lock;

static pthread_mutex_t *item_locks;

/* Which thread we assigned a connection to most recently. */
static int last_thread = -1;

 
/********函数声明************************/

static void thread_libevent_process(int fd, short which, void *arg);
static void register_thread_initialized(void);
static void wait_for_thread_registration(int nthreads);
 

void dispatch_conn_new(int sfd, enum conn_states init_state, int event_flags,
                       int read_buffer_size, enum network_transport transport);




/************　队列的操作 ******************************************************************************/

/*
 *  初始化队列
 */
static void cq_init(CQ *cq) {
    pthread_mutex_init(&cq->lock, NULL);
    pthread_cond_init(&cq->cond, NULL);
    cq->head = NULL;
    cq->tail = NULL;
}

/*
 * Looks for an item on a connection queue, but doesn't block if there isn't
 * one.
 * Returns the item, or NULL if no item is available
 */
static CQ_ITEM *cq_pop(CQ *cq) {
    CQ_ITEM *item;

    pthread_mutex_lock(&cq->lock);
    item = cq->head;
    if (NULL != item) {
        cq->head = item->next;
        if (NULL == cq->head)
            cq->tail = NULL;
    }
    pthread_mutex_unlock(&cq->lock);

    return item;
}

/*
 * Adds an item to a connection queue.
 * 在连接队列中增加一个项
 */
static void cq_push(CQ *cq, CQ_ITEM *item) {
    item->next = NULL;

    pthread_mutex_lock(&cq->lock);
    // 在队尾添加
    if (NULL == cq->tail)
        cq->head = item;
    else
        cq->tail->next = item;
    cq->tail = item;
    pthread_cond_signal(&cq->cond);
    pthread_mutex_unlock(&cq->lock);
}

/*
 * Frees a connection queue item (adds it to the freelist.)
 */
static void cqi_free(CQ_ITEM *item) {
    pthread_mutex_lock(&cqi_freelist_lock);
    item->next = cqi_freelist;
    cqi_freelist = item;
    pthread_mutex_unlock(&cqi_freelist_lock);
}

/*
 * Returns a fresh connection queue item.
 */
static CQ_ITEM *cqi_new(void) {
    CQ_ITEM *item = NULL;
    pthread_mutex_lock(&cqi_freelist_lock);
    if (cqi_freelist) {
        item = cqi_freelist;
        cqi_freelist = item->next;
    }
    pthread_mutex_unlock(&cqi_freelist_lock);

    if (NULL == item) {
        int i;

        /* Allocate a bunch of items at once to reduce fragmentation */
        item = (CQ_ITEM*) malloc(sizeof(CQ_ITEM) * ITEMS_PER_ALLOC);
        if (NULL == item)
            return NULL;

        /*
         * Link together all the new items except the first one
         * (which we'll return to the caller) for placement on
         * the freelist.
         */
        for (i = 2; i < ITEMS_PER_ALLOC; i++)
            item[i - 1].next = &item[i];

        pthread_mutex_lock(&cqi_freelist_lock);
        item[ITEMS_PER_ALLOC - 1].next = cqi_freelist;
        cqi_freelist = &item[1];
        pthread_mutex_unlock(&cqi_freelist_lock);
    }

    return item;
}

/**************************************************************************************************/

 
void STATS_LOCK() {
    pthread_mutex_lock(&stats_lock);
}

void STATS_UNLOCK() {
    pthread_mutex_unlock(&stats_lock);
}

/*
 *
 * 分发新的连接到线程池中的一个线程中, 其实就是在一个线程的工作队列中加入一个
 * 工作任务, 并通过管道给相应的线程发送信号
 *
 */
void dispatch_conn_new(int sfd, enum conn_states init_state, int event_flags,
                       int read_buffer_size, enum network_transport transport) {
Debug(" dispatch_conn_new begin... ");
    CQ_ITEM *item = cqi_new();
    char buf[1];

    // 线程池中有多个线程, 每个线程都有一个工作队列, 线程所需要做的就是从工作队列中取出工作任务并执行, 只要队列为空线程就可以进入等待状态
    // 计算线程信息下标
    int tid = (last_thread + 1) % settings.num_threads;

    // LIBEVENT_THREAD threads 是一个全局数组变量
    LIBEVENT_THREAD *thread = threads + tid; // 定位到下一个线程信息

    last_thread = tid;

    item->sfd = sfd;
    item->init_state = init_state;
    item->event_flags = event_flags;
    item->read_buffer_size = read_buffer_size;
    item->transport = transport;

    // 将工作任务放入对应线程的工作队列中
    cq_push(thread->new_conn_queue, item);

    //MEMCACHED_CONN_DISPATCH(sfd, thread->thread_id);

    // 这里向一个熟睡的线程写了一个字符: char buf[1]
    // 当管道中被写入数据后, libevent 中的注册事件会被触发, thread_libevent_process() 函数会被调用. 因为在 setup_thread() 中线程中管道描述符被设置到 event 中, 并注册到 libevent 中
    buf[0] = 'c';
    if (write(thread->notify_send_fd, buf, 1) != 1) {
        perror("Writing to thread notify pipe");
    }

}
 
/*.
 *     创建启动线程
 */
static void create_worker(void *(*func)(void *), void *arg) {
    pthread_t       thread;
    pthread_attr_t  attr;
    int             ret;

    pthread_attr_init(&attr);

    if ((ret = pthread_create(&thread, &attr, func, arg)) != 0) {
        fprintf(stderr, "Can't create thread: %s\n",
                strerror(ret));
        exit(1);
    }
}

/****************************** LIBEVENT THREADS *****************************/

/*
 *
 * 填充 LIBEVENT_THREAD 结构体, 其中包括:
 *    填充 struct event
 *    初始化线程工作队列
 *    初始化互斥量
 *     等
 */
static void setup_thread(LIBEVENT_THREAD *me) {
Debug(".......... setup_thread begin ..........");
    me->base = event_init();
    if (! me->base) {
        fprintf(stderr, "Can't allocate event base\n");
        exit(1);
    }

    // 在线程数据结构初始化的时候, 为 me->notify_receive_fd 读管道注册读事件, 回调函数是 thread_libevent_process()
    event_set(&me->notify_event, me->notify_receive_fd,
              EV_READ | EV_PERSIST, thread_libevent_process, me);
    event_base_set(me->base, &me->notify_event);

    if (event_add(&me->notify_event, 0) == -1) {
        fprintf(stderr, "Can't monitor libevent notify pipe\n");
        
    }
    
#ifdef _MIGRANT_USE_REDIS 
    /*  与redis建立来链接
    if( -1 == redis_connect(me->redis_conn,me->base,"127.0.0.1", 6379) ){
        Debug("redis_connect error");
        return -1;
    }
    */
    
    me->redis_conn  = redisConnect("127.0.0.1",9527); 
     if(me->redis_conn != NULL && me->redis_conn->err) 
     {   
         fprintf(stderr,"connection redis  error: %s\n",me->redis_conn->errstr); 
         exit(1); ; 
     }
     
    
#endif
    
    // 初始化该线程的工作队列
    me->new_conn_queue =(struct conn_queue*) malloc(sizeof(struct conn_queue));
    if (me->new_conn_queue == NULL) {
        perror("Failed to allocate memory for connection queue");
        exit(EXIT_FAILURE);
    }
    cq_init(me->new_conn_queue);

    // 初始化该线程的状态互斥量
    if (pthread_mutex_init(&me->stats.mutex, NULL) != 0) {
        perror("Failed to initialize mutex");
        exit(EXIT_FAILURE);
    }

}

/*
 * 线程函数入口, 启动事件循环
 */
static void *worker_libevent(void *arg) {
Debug(".......... worker_libevent begin ..........  ");
Debug(" ########## work  thread   begin,  waiting  to  be assigned  new work ......  ########## ");

    LIBEVENT_THREAD *me =( LIBEVENT_THREAD*) arg;
    register_thread_initialized();

    // 进入事件循环
    event_base_loop(me->base, 0);
//Debug(" worker_libevent end .  thread create over ......");
    return NULL;
}

/*
 *
 * 当管道有数据可读的时候会触发此函数的调用
 */
static void thread_libevent_process(int fd, short which, void *arg) {
Debug(".......... thread_libevent_process begin ..........");
    LIBEVENT_THREAD *me = (LIBEVENT_THREAD *)arg;
    CQ_ITEM *item;
    char buf[1];

    if (read(fd, buf, 1) != 1){
      //  if (settings.verbose > 0)
            fprintf(stderr, "Can't read from libevent pipe\n");
	}
	
    switch (buf[0]) {
    case 'c':
Debug(" ########## work thread  recv from pipe  'c',  begin work to read  new client from queue ##########");
    // 取出一个任务
    item = cq_pop(me->new_conn_queue);

    if (NULL != item) {
        // 为新的请求建立一个连接结构体. 连接其实已经建立, 这里只是为了填充连接结构体. 最关键的动作是在 libevent 中注册了事件, 回调函数是 event_handler()
        conn *c = conn_new(item->sfd, item->init_state, item->event_flags,
                           item->read_buffer_size, item->transport, me->base);
        if (c == NULL) {
            if (IS_UDP(item->transport)) {
                fprintf(stderr, "Can't listen for events on UDP socket\n");
                exit(1);
            } else {
               /* if (settings.verbose > 0) {
                    fprintf(stderr, "Can't listen for events on fd %d\n",
                        item->sfd);
                } */
                close(item->sfd);
            }
        } else {
            c->thread = me;
        }
        cqi_free(item);
    }
        break;

    /* we were told to flip the lock type and report in */
    case 'd':
Debug("case d");
   /* Debug("####################$$$$$$$$$$$$$$$$$$$$$$$");
      CQ_ITEM* item_tmp = me->new_conn_queue->head;
       if( NULL != item_tmp){
            do{
        
            }while(item_tmp->next != NULL);
        }
   */
        
        break;

    case 'g':
Debug("case g");
   // me->item_lock_type = ITEM_LOCK_GLOBAL;
    register_thread_initialized();
        break;
    }
}

 /*
 * Returns true if this is the thread that listens for new TCP connections.
 */
int is_listen_thread() {
    return pthread_self() == dispatcher_thread.thread_id;
}

/*
 * 初始化线程子系统, 创建工作线程
 *  需准备的线程数
 *  分发线程
 */
void thread_init(int nthreads, struct event_base *main_base) {
Debug(".......... thread_init begin ..........");
    int         i;
    int         power;

    // 互斥量初始化
    pthread_mutex_init(&cache_lock, NULL);
    pthread_mutex_init(&stats_lock, NULL);

    pthread_mutex_init(&init_lock, NULL);
    pthread_cond_init(&init_cond, NULL);

    pthread_mutex_init(&cqi_freelist_lock, NULL);
    cqi_freelist = NULL;

/*
    //  Want a wide lock table, but don't waste memory 
    // 锁表?
    if (nthreads < 3) {
        power = 10;
    } else if (nthreads < 4) {
        power = 11;
    } else if (nthreads < 5) {
        power = 12;
    } else {
        // 2^13
        // 8192 buckets, and central locks don't scale much past 5 threads 
        power = 13;
    }

    // 预申请那么多的锁, 拿来做什么
    // hashsize = 2^n
 //   item_lock_count = hashsize(power);

    item_locks = calloc(item_lock_count, sizeof(pthread_mutex_t));
    if (! item_locks) {
        perror("Can't allocate item locks");
        exit(1);
    }
  
    // 初始化
    for (i = 0; i < item_lock_count; i++) {
        pthread_mutex_init(&item_locks[i], NULL);
    }
    pthread_key_create(&item_lock_type_key, NULL);
    pthread_mutex_init(&item_global_lock, NULL);

*/
    // LIBEVENT_THREAD 是结合 libevent 使用的结构体, event_base, 读写管道
    threads =  (LIBEVENT_THREAD*)calloc(nthreads, sizeof(LIBEVENT_THREAD));
    if (! threads) {
        perror("Can't allocate thread descriptors");
        exit(1);
    }

    // main_base 是分发任务的线程, 即主线程
    dispatcher_thread.base = main_base;
	dispatcher_thread.thread_id = pthread_self();

	// 管道, libevent 通知用的
	for (i = 0; i < nthreads; i++) {
    	int fds[2];
        if (pipe(fds)) {
            perror("Can't create notify pipe");
            exit(1);
        }

        // 读管道
        threads[i].notify_receive_fd = fds[0];
        // 写管道
        threads[i].notify_send_fd = fds[1];

Debug(" ########## setup thread[%d] ##########",i);
        // 初始化线程信息数据结构, 其中就将 event 结构体的回调函数设置为 thread_libevent_process()
        setup_thread(&threads[i]);
        /* Reserve three fds for the libevent base, and two for the pipe */
        stats.reserved_fds += 5;    
    }

    // 创建并初始化线程, 线程的代码都是 worker_libevent
    for (i = 0; i < nthreads; i++) {
        create_worker(worker_libevent, &threads[i]);
    }

    /* Wait for all the threads to set themselves up before returning. */
    pthread_mutex_lock(&init_lock);
    // wait_for_thread_registration() 是 pthread_cond_wait 的调用
    wait_for_thread_registration(nthreads);
    pthread_mutex_unlock(&init_lock);
}

/*
 * 让线程进入为某个条件 init_cond 等待的状态
 */
static void wait_for_thread_registration(int nthreads) {
    // pthread_cond_wait() 按习惯是需要被包含在 while 循环中.
    // 这里的 init_count < nthreads 是为了统一唤醒所有的进程
    while (init_count < nthreads) {
        pthread_cond_wait(&init_cond, &init_lock);
    }
}

static void register_thread_initialized(void) {
    pthread_mutex_lock(&init_lock);
    init_count++;
    pthread_cond_signal(&init_cond);
    pthread_mutex_unlock(&init_lock);
}



/**************************************关于定时器**********************************/
/*
void check_total(int fd, short events, void *ctx) {
Debug(" check_total begin ............");   
    char buf[1];
    buf[0] = 'd';
    int i = 0;
    for(; i< settings.num_threads;++i){
        LIBEVENT_THREAD *thread = threads + i; // 定位到下一个线程信息
        if (write(thread->notify_send_fd, buf, 1) != 1) {
            perror("Writing to thread notify pipe");
        }  
    }    
}
*/
