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
