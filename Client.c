#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>
#include <stdbool.h>
#define SHMKEY 75
#define MSGMAXN 70

typedef struct{
    time_t t;
    int pid;
    char text[15];
}MSG;

typedef struct{
    int pid;
    bool state;
    bool send_state;
}PROCESS;

typedef struct{
    int spid;
    int psum;
    int temp;
    int msgnum;
    PROCESS process[15];
    MSG msg[MSGMAXN];
}MEM;

MEM *addr;   //共享存储区首地址
int shmid;
int i1,i2;   //i1是消息的写指针，i2是消息的读指针
int inx=0;   //进程信息process数组的相对号
int recv_flag = 1;  //是否接收消息

void init()  //初始化
{
    time_t t;
    struct tm *p;
    time(&t);
    p = gmtime(&t);
    
    printf("%d:%d:%d  %d:  你已进入群聊\n",p->tm_hour+8,p->tm_min,p->tm_sec,getpid());
    
    int shmid=shmget(SHMKEY,1024,0777);      //打开共享存储区
    if (shmid == -1)
    {
        perror("shmget");       //输出上一个函数错误的原因
        exit(-2);
    }
    //映射共享内存
    addr = (MEM*)shmat(shmid, 0, 0);
    if (addr == (void *)-1)
    {
        perror("shmat");
        exit(-3);
    }
    
    lockf(1,1,0);
    (*addr).process[(*addr).psum].pid=getpid();
    (*addr).process[(*addr).psum].state=1;
    (*addr).process[(*addr).psum].send_state=1;
    inx=(*addr).psum;
    (*addr).psum++;
    
    int in1=(*addr).msgnum;
    strcpy((*addr).msg[in1].text,"进入群聊\n");
    time(&(*addr).msg[in1].t);
    (*addr).msg[in1].pid=getpid();
    (*addr).msgnum++;
    lockf(1,0,0);
}

void signal_to_all1()
{
	int n = (*addr).psum;
	for (int i = 0;i < n;i++)
	{
		if ((*addr).process[i].state == 1) {
			kill((*addr).process[i].pid, 17);
		}
	}
}


void send()
{
    char c[15];
    while (1)
    {
        fgets(c,15,stdin);      
        if (!(*addr).process[inx].send_state) {
            printf("你已被禁言\n消息发送失败\n");
            continue;
        }     //被禁言，忽略发送内容，进入下一次循环
        
        if (strstr(c,"1#")!=NULL) {recv_flag = 0;continue;}   //不接受消息
        if (strstr(c,"2#")!=NULL) {recv_flag = 1;continue;}   //接收消息
		if (strstr(c, "*") != NULL) { signal_to_all1();continue; } //撤回消息
        lockf(1,1,0);
        i1=(*addr).msgnum;
        strcpy((*addr).msg[i1].text,c);
        time(&(*addr).msg[i1].t);
        (*addr).msg[i1].pid=getpid();
        
        kill((*addr).spid,SIGUSR1);     //向server发送信号，提醒server检查消息
        (*addr).msgnum++;
        lockf(1,0,0);
    }
}

void *recv(void* arg)       //接收消息
{
    i2=0;
    struct tm *p;
    while (1)
    {       
        while (((*addr).msgnum) <= i2)
        {
            sleep(5);
        }
        if (!recv_flag) {
            i2++;
            sleep(10);
            continue;
        }               //不接收信息
        
        lockf(1,1,0);
        
        if ((*addr).msg[i2].pid==getpid())
        {
            lockf(1,0,0);
            (i2)++;
            continue;
        }
        
        p = gmtime(&(*addr).msg[i2].t);
        printf("%d:%d:%d  ",p->tm_hour+8,p->tm_min,p->tm_sec);
        printf("%d: ",(*addr).msg[i2].pid);
        printf(" %s",(*addr).msg[i2].text);
        lockf(1,0,0);
        (i2)++;
    }
}

void pexit(int sig)
{
    lockf(1,1,0);   
    (*addr).process[inx].state=0;     //不在线状态
    
    int in1=(*addr).msgnum;
    time(&(*addr).msg[in1].t);
    (*addr).msg[in1].pid=getpid();
    
    if (sig==SIGUSR2) 
    {
        strcpy((*addr).msg[in1].text,"已被踢出群聊\n");
        printf("你已被踢出群聊\n");
    }
    else 
    {
        strcpy((*addr).msg[in1].text,"已退出群聊\n");
        printf("你已退出群聊\n");
    }
    (*addr).msgnum++;
    lockf(1,0,0);
    
    exit(0);
}

void func(int signo)
{
    i2 -= (*addr).temp;     //减去删除的消息数
}

void func1(int signo)
{   
	i2++;
	                                          
}

int main()
{
	
    init();
    signal(SIGUSR2,pexit);
    signal(SIGINT,pexit);
    signal(16,func);
	signal(17, func1);

    pthread_t id1;
    int err = pthread_create(&id1, NULL, recv, NULL);
    if (err != 0)  
        printf("can't create thread 1: %d\n", err);
        
    send();
    
    pthread_join(id1, 0); //等待线程结束
    return 0;
}

