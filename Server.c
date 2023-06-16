#include<sys/types.h>
#include<sys/shm.h>
#include<sys/ipc.h>
#include<unistd.h>  
#include<stdlib.h>   
#include<stdio.h>
#include<sys/wait.h> 
#include<signal.h> 
#include<string.h>
#include <stdbool.h>
#include<time.h>
#include <pthread.h>
#define SHMKEY 75   //共享存储区的名字
#define MSGMAXN 70  //最大消息数

typedef struct{
    time_t t;           //时间
    int pid;            //进程号
    char text[15];      //文本内容
}MSG;               //消息

typedef struct{
    int pid;                //进程号
    bool state;             //是否在线
    bool send_state;        //是否被禁言
}PROCESS;           //进程信息

typedef struct{
    int spid;               //server进程号
    int psum;               //进程总数
    int temp;               //临时变量
    int msgnum;             //消息数量
    PROCESS process[15];  //进程数组
    MSG msg[MSGMAXN];    //消息数组
}MEM;                   //内存内容

int shmid;
MEM *addr;    //共享存储区首地址

void init()     //初始化
{
    printf("server pid:%d\n",getpid());
    shmid=shmget(SHMKEY,1024,0777|IPC_CREAT); //创建共享存储区
    system("ipcs -m");//相当于在SHELL下执行 ipcs -m命令，该命令是显示系统所有的共享内存 
    addr=(MEM*)shmat(shmid,0,0);        //获取首地址
    (*addr).psum=0;
    (*addr).msgnum=0;
    (*addr).spid=getpid();
}

void check(int signo)
{
    printf("checking...\n");
    char s[20][15];
    
    memset(s,'\0',200);
    
    FILE *file;
    char *FILE_NAME=(char*)"sensitive_word.bin";
    
    file = fopen(FILE_NAME,"rb");    //以rb方式打开文件
    if (file == NULL) perror("errno");
    
    int len;
    fseek(file, 0, SEEK_END);       //文件指针定位到文件末尾，偏移0个字节 
    len = ftell(file);              //获取文件长度, ftell函数用于得到文件位置指针当前位置相对于文件首的偏移字节数       
    
    fseek(file,0,SEEK_SET);         //将文件指针移回文件的开头 
    for (int i=0;i<len/15;i++)
        fread(s[i],15,1,file);      //读取文件内容，并存入s
        
    int curidx =((*addr).msgnum) - 1;
    char *current=(*addr).msg[curidx].text; 
    
    int n = len/15;
    for (int i=0;i<n;i++)
    {
        if(strstr(current,s[i])!=NULL)
        {
            printf("%d发送敏感词%s",(*addr).msg[curidx].pid,s[i]);
            strcpy((*addr).msg[curidx].text,"******\n");   //覆盖敏感词
            break;
        }
    }
}

void write_senwd()
{
    printf("请输入敏感词\n");
    char *FILE_NAME = (char*)"sensitive_word.bin";  //敏感词词库文件
    FILE *file;
    
    char s[15];
    file = fopen(FILE_NAME,"wb");   //创建文件，并以wb方式打开
    if (file == NULL) perror("errno");
    
    int i;    
    for (i=0;i<15;i++)
    {
        fgets(s,15,stdin);
        if (strstr(s,"1#")!=NULL) break;
        fwrite(&s,15,1,file);        //将敏感词写入文件
    }
    printf("写入敏感词成功\n\n");
    fclose(file);
}

void delete_senwd()
{
    printf("\n请输入敏感词\n");
    char a[30][15];
    int senwdnum;
    for(senwdnum=0;senwdnum<30;senwdnum++)
    {
        fgets(a[senwdnum],15,stdin);
        if (strstr(a[senwdnum],"2#")!=NULL) break;
    }
    
    char s[30][15];
    memset(s,'\0',200);
    
    FILE *file ,*file1;
    char *FILE_NAME=(char*)"sensitive_word.bin";
    
    file = fopen(FILE_NAME,"rb");    //以rb方式打开文件
    if (file == NULL) perror("errno");
    
    int len;
    fseek(file, 0, SEEK_END);       //文件指针定位到文件末尾，偏移0个字节 
    len = ftell(file);              //获取文件长度, ftell函数用于得到文件位置指针当前位置相对于文件首的偏移字节数    
        
    fseek(file,0,SEEK_SET);         //将文件指针移回文件的开头 
    for (int i=0;i<len/15;i++)
        fread(s[i],15,1,file);      //读取文件内容，并存入s  
        
    fclose(file);  
    remove(FILE_NAME);      //删除原文件
    
    file1 = fopen(FILE_NAME,"wb");     //新建文件
    for(int i=0;i<len/15;i++)
        for(int j=0;j<senwdnum;j++)
            if(strstr(a[j],s[i]) == NULL)
                fwrite(&s[i],15,1,file1);    
                
    printf("删除敏感词成功\n\n");
    fclose(file1);
}


void show_senwd()
{
    char s[30][15];
    memset(s,'\0',200);
    
    FILE *file;
    char *FILE_NAME=(char*)"sensitive_word.bin";
    file = fopen(FILE_NAME,"rb");    //以rb方式打开文件
    if (file == NULL) perror("errno");
    
    int len;
    fseek(file, 0, SEEK_END);       //文件指针定位到文件末尾，偏移0个字节 
    len = ftell(file);              //获取文件长度, ftell函数用于得到文件位置指针当前位置相对于文件首的偏移字节数       
    
    fseek(file,0,SEEK_SET);         //将文件指针移回文件的开头 
    for (int i=0;i<len/15;i++)
        {
            fread(s[i],15,1,file);  //读取文件内容，并存入s
            printf("%d---%s",i+1,s[i]);
        }      
    printf("\n");
}

void manage()
{
    int psum=(*addr).psum;
    printf("\n所有进程及状态：\n");
    for (int i=0;i<psum;i++)
    {
        printf("%d ",(*addr).process[i].pid);
        if ((*addr).process[i].state) printf("\t在线");
        else printf("\t离开");
        if ((*addr).process[i].send_state) printf("\t可发言\n");
        else printf("\t被禁言\n");
    }
    
    int ctrl;
    printf("\n输入管理信号：1--移除进程\n2--禁言\n3--解除禁言\n");
    scanf("%d",&ctrl);
    
    if (ctrl==1) {
        int n;
        printf("输入要移除的进程号：");
        scanf("%d",&n);
        kill(n,SIGUSR2);                      //向要移除的进程发送信号
        printf("\n进程%d已移除\n",n);
    }
    if (ctrl==2 ||ctrl==3) {
        int n;
        printf("输入进程号:");
        scanf("%d",&n);
        for(int i=0;i<psum;i++)
        {
            if ((*addr).process[i].pid==n){
                if (ctrl==2) (*addr).process[i].send_state = 0;
                else (*addr).process[i].send_state = 1;
                break;
            }                         //改变进程是否被禁言状态
        }
    }
}

void signal_to_all()
{
    int n = (*addr).psum;
    for (int i=0;i<n;i++)
    {
        if ((*addr).process[i].state == 1) {
            kill((*addr).process[i].pid,16);
        }
    }
}  

void *cleanmsg(void* arg)
{
    MSG *msg;
    time_t t;
    struct tm *p,*msg_p;  //通过tm结构来获得日期和时间
    int i;
    
    while(1)
    {
        time(&t);       //此函数会返回从公元 1970 年1 月1 日的UTC 时间从0 时0 分0 秒算起到现在所经过的秒数。如果t 并非空指针的话，此函数也会将返回值存到t 指针所指的内存。
        for (i = 0;i<(*addr).msgnum;i++)
        {
            double fl=difftime(t,(*addr).msg[i].t);     //返回两个time_t型变量之间的时间间隔
            if (fl<=120.0) break;                      //时间差小于120s
        }
        if (i == 0)
        {
            sleep(10);
            continue;
        }                               //没有可清除msg
        lockf(1,1,0);       //锁定屏幕输出
 
        (*addr).msgnum=(*addr).msgnum-i;     //改变消息数
        (*addr).temp=i;       //清除的消息数
        signal_to_all();  //向所有在线进程发送信号
        lockf(1,0,0);       //解锁.
      
        sleep(10);
    }
}

void EXIT()
{
    int psum=(*addr).psum;
    for (int i=0;i<psum;i++)
    {
        kill((*addr).process[i].pid,SIGUSR2);  
    }
    exit(0);
}

int main()
{
    init();
    signal(SIGUSR1,check);
    printf("1#---添加敏感词\n2#---删除敏感词\n3#---显示敏感词\n4#---管理线程\n5#---退出\n");
    
    pthread_t id1;
    int err = pthread_create(&id1, NULL, cleanmsg, NULL);
    if (err != 0)  
        printf("can't create thread 1: %d\n", err);
    char control[10];      //控制字符
    while (1)
    {
        fgets(control,10,stdin);
        if (strstr(control,"1#")!=NULL) write_senwd();
        if (strstr(control,"2#")!=NULL) delete_senwd();
        if (strstr(control,"3#")!=NULL) show_senwd();
        if (strstr(control,"4#")!=NULL) manage();
        if (strstr(control,"5#")!=NULL) {EXIT();exit(0);}
    }
    shmctl(shmid,IPC_RMID,0);     //撤消共享存储区，归还资源
 exit(0);
}

