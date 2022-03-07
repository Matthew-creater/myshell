#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <cerrno>
#include <dirent.h> 

#define buffsize 255
#define maxargs 128
#define maxline 20

/* Job states */
#define UNDEF 0 
#define FG 1    
#define BG 2    
#define ST 3    

char prompt[]="myshell> ";
char buf[buffsize];
int argc;
char* argv[maxargs];
char curPath[buffsize];  
char history[maxline][buffsize];
int cmd_num = 0;   

int getcommand(char *buf);
void parseline(char *cmd);
void eval(int argc,char* argv[]);
int CD(int argc);
int HISTORY(int argc);
int outRedirect(int argc,int inter,char* argv[]);
int inRedirect(int argc,int inter,char* argv[]);
int reoutRedirect(int argc,int inter,char* argv[]);
int Pipecommand(char* buf);
int backMission(int argc,char* argv[]);
void mytop();


int main()
{   
    int LOOP = 1;
    while(LOOP)
    {
        printf("%s",prompt);
        if(getcommand(buf))
        {   
            if(strcmp(buf,"exit")==0)
            {   
                LOOP = 0 ;
                exit(0);
                break;
            }
            else
            {
                strcpy(history[cmd_num],buf);
                cmd_num++;
                parseline(buf);
                eval(argc,argv);                
            }
        }
    }
    return 0;
}

int getcommand(char *buf)
{
    memset(buf,0,buffsize);
    int length;

    fgets(buf,buffsize,stdin);
    length = strlen(buf);
    buf[length-1] = '\0';
    return strlen(buf);
}

void parseline(char* cmd)
{
    int i,j,k;
    int len = strlen(cmd);
    int num=0;
    for(i=0;i<maxargs;i++)
    {
        argv[i] = NULL;
    }

    char tmp[buffsize];
    j=-1;
    for(i=0;i<=len;i++)
    {
        if(cmd[i]==' '||i==len)
        {
            if(i-1>j)
            {
                cmd[i]='\0';
                argv[num++] = cmd+j+1;
            }
            j = i;
        }
    }
    argc = num;
    argv[argc] = NULL;
}

void eval(int argc,char* argv[])
{   
    pid_t pid,pid2;
    int i,j;
    int bg_flag = 0;
   /*program_command*/
   //'>'
    for(i=0;i<argc;i++)
    {
        if(strcmp(argv[i],">") == 0)
        {   
            int tmp = outRedirect(argc,i,argv);
            return;
        }
    }
    //'<'
    for(i=0;i<argc;i++)
    {
        if(strcmp(argv[i],"<") == 0)
        {   
            int tmp = inRedirect(argc,i,argv);
            return;
        }
    }
    //'>>'
    for(i=0;i<argc;i++)
    {
        if(strcmp(argv[i],">>") == 0)
        {   
            int tmp = reoutRedirect(argc,i,argv);
            return;
        }
    }
    //pipe
    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], "|") == 0) {
            strcpy(buf, history[cmd_num-1]);
            if((pid = fork()) < 0){  //创建子进程
                printf("fork error\n");
                return ;
            }
            if (pid == 0)
            {
                int tmp=Pipecommand(buf);
            }
            else
            {
                int status;
                if (waitpid(pid, &status, 0) == -1)
                {
                    printf("wait for child process error\n");
                }
            }
            return;
        }
    }
    //后台
    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], "&") == 0) {
            bg_flag = 1;
            argv[argc-1] =NULL;
            argc = argc -1;
            //int tmp = backMission(argc,argv);
            //return;
        }
    }
        
    /*内置命令*/
    if(strcmp(argv[0],"cd")==0)
    {   
        int res = CD(argc);
        if(res==0) printf("cd指令错误\n");
    }
    /*else if(strcmp(argv[0],"exit")==0)
    {   
    printf("exit:\n");
    exit(0);
    }*/
    else if(strcmp(argv[0],"history")==0)
    {   
        int res = HISTORY(argc);
        if(res ==0) printf("history指令错误\n");
    }
    else if(strcmp(argv[0],"mytop")==0)
    {
        mytop();
    }
    else
    {   
        if((pid=fork())==-1)
        {
            printf("子进程创建失败\n");
            return ;
        }
        else if(pid == 0)
        {   
            if(bg_flag)
            {
                int fd1 = open("/dev/null", O_RDONLY);
                dup2(fd1, 0);
                dup2(fd1, 1);
                dup2(fd1, 2);
                signal(SIGCHLD, SIG_IGN);
            }
            execvp(argv[0], argv);
            printf("%s: 命令输入错误\n", argv[0]);
            exit(1);

        }
        else
        {
            if(bg_flag)
            {
                printf("[process id %d]\n", pid); 
                return ;       //若为后台程序，则输出进程号
            }
            int status;
            waitpid(pid, &status, 0);       // 等待子进程返回
            int err = WEXITSTATUS(status);  // 读取子进程的返回码
            if (err) { 
                printf("Error %d: %s\n", errno,strerror(err));
            }
        }
    }
    

}

int CD(int argc)
{
    int res ;
    if(argc == 2) 
    {
        if(!chdir(argv[1])) res = 1;
        else res = 0;
    }
    else{
        res = 0;
    }

    if(getcwd(curPath, buffsize)==NULL)
    {
        printf("getcwd failed\n");
        res =0;
    }          
    else printf("%s\n",curPath);
    return res;    
}

int HISTORY(int argc)
{   
    if(argc!=2) 
    {
        printf("history failed\n");
        return 0;
    }
    int res = 1;
    int n = atoi(argv[argc-1]);
    int i;
    for (i = n; i > 0; i--) {
        if( cmd_num - i < 0)
        {   
            continue;
        }
        printf("%s\n", history[cmd_num - i]);
    }
    return res;
}

int outRedirect(int argc,int inter,char* argv[])
{
    char outFile[buffsize];
    memset(outFile, 0x00, buffsize);
    int i,j;
    int redi_num = 0;
    for(i=0;i<argc;i++)
    {   
        if(strcmp(argv[i],">")==0)
        {
            redi_num += 1 ;
        }
    }
    if(redi_num != 1)
    {
        printf("wrong Redirect command\n");
        return 0;
    }
    if(inter==argc-1) 
    {
        printf("lack of output-file name\n");
        return 0;
    }
    if(argc-inter>2) 
    {
        printf("to many file\n");
        return 0;
    }
    strcpy(outFile,argv[inter+1]);
    for(j=inter;j<argc;j++)
    {
        argv[inter] =NULL;
    }

    pid_t pid;
    
    if((pid=fork())==-1)
    {
        printf("子进程创建失败\n");
        return 0;
    }
    else if(pid == 0)
    {
        int fd;
        int oldfd;
        fd = open(outFile, O_WRONLY|O_CREAT|O_TRUNC, 777);
        // 文件打开失败
        if (fd < 0) {
            printf("open error\n");
            exit(1);
        }
        if(dup2(fd, STDOUT_FILENO) == -1)
        {
            printf("dup2 error\n");
            exit(1);
        } 
        execvp(argv[0], argv);
        if (fd != STDOUT_FILENO) {
            close(fd);
        }      
        printf("%s: 命令输入错误\n", argv[0]);
        exit(1);
    }
    int status;
    waitpid(pid, &status, 0);       // 等待子进程返回
    int err = WEXITSTATUS(status);  // 读取子进程的返回码
    if (err) { 
        printf("Error %d: %s\n", errno,strerror(err));
    }
    
    return 0;
}

int inRedirect(int argc,int inter,char* argv[])
{
    char inFile[buffsize];
    memset(inFile, 0x00, buffsize);
    int i,j;
    int redi_num = 0;
    for(i=0;i<argc;i++)
    {   
        if(strcmp(argv[i],"<")==0)
        {
            redi_num += 1 ;
        }
    }
    if(redi_num != 1)
    {
        printf("wrong Redirect command\n");
        return 0;
    }
    strcpy(inFile,argv[inter+1]);
    for(j=inter;j<argc;j++)
    {
        argv[inter] =NULL;
    }

    pid_t pid;
    
    if((pid=fork())==-1)
    {
        printf("子进程创建失败\n");
        return 0;
    }
    else if(pid == 0)
    {
        int fd;
        int oldfd;
        fd = open(inFile, O_RDONLY, 777);
        // 文件打开失败
        if (fd < 0) {
            printf("open error\n");
            exit(1);
        }
        if(dup2(fd, STDIN_FILENO) == -1)
        {
            printf("dup2 error\n");
            exit(1);
        } 
        execvp(argv[0], argv);
        if (fd != STDOUT_FILENO) {
            close(fd);
        }      
        printf("%s: 命令输入错误\n", argv[0]);
        exit(1);
    }
    int status;
    waitpid(pid, &status, 0);       // 等待子进程返回
    int err = WEXITSTATUS(status);  // 读取子进程的返回码
    if (err) { 
        printf("Error %d: %s\n", errno,strerror(err));
    }
    
    return 0;
}

int reoutRedirect(int argc,int inter,char* argv[])
{
    char outFile[buffsize];
    memset(outFile, 0x00, buffsize);
    int i,j;
    int redi_num = 0;
    for(i=0;i<argc;i++)
    {   
        if(strcmp(argv[i],">>")==0)
        {
            redi_num += 1 ;
        }
    }
    if(redi_num != 1)
    {
        printf("wrong Redirect command\n");
        return 0;
    }
    if(inter==argc-1) 
    {
        printf("lack of output-file name\n");
        return 0;
    }
    if(argc-inter>2) 
    {
        printf("to many file\n");
        return 0;
    }
    strcpy(outFile,argv[inter+1]);
    for(j=inter;j<argc;j++)
    {
        argv[inter] =NULL;
    }

    pid_t pid;
    
    if((pid=fork())==-1)
    {
        printf("子进程创建失败\n");
        return 0;
    }
    else if(pid == 0)
    {
        int fd;
        int oldfd;
        fd = open(outFile, O_WRONLY|O_APPEND|O_CREAT|O_APPEND, 777);
        // 文件打开失败
        if (fd < 0) {
            printf("open error\n");
            exit(1);
        }
        if(dup2(fd, STDOUT_FILENO) == -1)
        {
            printf("dup2 error\n");
            exit(1);
        } 
        execvp(argv[0], argv);
        if (fd != STDOUT_FILENO) {
            close(fd);
        }      
        printf("%s: 命令输入错误\n", argv[0]);
        exit(1);
    }
    int status;
    waitpid(pid, &status, 0);       // 等待子进程返回
    int err = WEXITSTATUS(status);  // 读取子进程的返回码
    if (err) { 
        printf("Error %d: %s\n", errno,strerror(err));
    }
    
    return 0;
}

int Pipecommand(char* buf)
{
    int fd[2];
    int i;
    int pos;
    pid_t pid;
    
    for(i=0;i<strlen(buf);i++)
    {
        if(buf[i]=='|'&&buf[i+1]==' ')
        {
            pos = i;
            break;
        }
    }
    char readpipe[buffsize];
    memset(readpipe,0,buffsize);
    char writepipe[buffsize];
    memset(writepipe,0,buffsize);
    for(i=0;i<strlen(buf);i++)
    {
        if(i<pos)
        {
            writepipe[i] = buf[i];
        }
        else if(i>pos)
        {
            readpipe[i-pos-1] = buf[i];
        }
    }
    writepipe[pos]='\0';
    readpipe[strlen(buf)-pos-1] = '\0';


    if (pipe(fd) < 0) {
        printf("pipe failed\n");
        exit(1);
    }

    pid = fork();
    if (pid < 0) {
        printf("创建子进程失败\n");
        exit(1);
    }
    if(pid==0){
        close(fd[0]);
        close(1);
        dup(fd[1]);//fd[1]管道写入端，映射到标准输出1
        close(fd[1]);
        parseline(writepipe);
        //printf("%s %d\n",argv[0],argc);
        //eval(argc,argv);
        execvp(argv[0], argv);
        return 0;
    }else{
        int status;
        waitpid(pid, &status, 0);       // 等待子进程返回
        int err = WEXITSTATUS(status);  // 读取子进程的返回码
        if (err) { 
            printf("Error: %s\n", strerror(err));
        }

        close(fd[1]);
        close(0);
        dup(fd[0]);//fd[0]管道读入端，映射到标准输入0
        close(fd[0]);
        //printf("%s %d\n",argv[0],argc);
        parseline(readpipe);
        //eval(argc,argv);
        execvp(argv[0], argv);
        return 0;
    }

}



void mytop() {
    FILE *fp = NULL;                   
    char buff[255];

    /* 获取内容一: 
       总体内存大小，
       空闲内存大小，
       缓存大小。 */
    fp = fopen("/proc/meminfo", "r");   // 以只读方式打开meminfo文件
    fgets(buff, 255, (FILE*)fp);        // 读取meminfo文件内容进buff
    fclose(fp);

    // 获取 pagesize
    int i = 0, pagesize = 0;
    while (buff[i] != ' ') {
        pagesize = 10 * pagesize + buff[i] - 48;
        i++;
    }

    // 获取 页总数 total
    i++;
    int total = 0;
    while (buff[i] != ' ') {
        total = 10 * total + buff[i] - 48;
        i++;
    }

    // 获取空闲页数 free
    i++;
    int free = 0;
    while (buff[i] != ' ') {
        free = 10 * free + buff[i] - 48;
        i++;
    }

    // 获取最大页数量largest
    i++;
    int largest = 0;
    while (buff[i] != ' ') {
        largest = 10 * largest + buff[i] - 48;
        i++;
    }

    // 获取缓存页数量 cached
    i++;
    int cached = 0;
    while (buff[i] >= '0' && buff[i] <= '9') {
        cached = 10 * cached + buff[i] - 48;
        i++;
    }



    // 总体内存大小 = (pagesize * total) / 1024 单位 KB
    int totalMemory  = pagesize / 1024 * total;
    // 空闲内存大小 = pagesize * free) / 1024 单位 KB
    int freeMemory   = pagesize / 1024 * free;
    // 缓存大小    = (pagesize * cached) / 1024 单位 KB
    int cachedMemory = pagesize / 1024 * cached;

    printf("totalMemory  is %d KB\n", totalMemory);
    printf("freeMemory   is %d KB\n", freeMemory);
    printf("cachedMemory is %d KB\n", cachedMemory);

    /* 2. 获取内容2
        进程和任务数量
     */
    unsigned int procsnum, tasksnum;
    int total_n;
    if ((fp = fopen("/proc/kinfo", "r")) == NULL)
    {
        fprintf(stderr, "opening /proc/kinfo failed\n");
        exit(1);
    }

    if (fscanf(fp, "%u %u", &procsnum, &tasksnum) != 2)
    {
        fprintf(stderr, "reading from /proc/kinfo failed");
        exit(1);
    }

    fclose(fp);
    

    total_n = (int)(procsnum + tasksnum);

    // /* 3. 获取psinfo中的内容 */
    DIR *d;
    struct dirent *dir;
    d = opendir("/proc");
    long totalTicks = 0;
    long freeTicks = 0;
    if (d) {
        while ((dir = readdir(d)) != NULL) {                   // 遍历proc文件夹
            if (strcmp(dir->d_name, ".") != 0 && 
                strcmp(dir->d_name, "..") != 0) {
                    char path[255];
                    memset(path, 0x00, 255);
                    strcpy(path, "/proc/");
                    strcat(path, dir->d_name);          // 连接成为完成路径名
                    struct stat s;
                    if (stat (path, &s) == 0) {
                        if (S_ISDIR(s.st_mode)) {       // 判断为目录
                            strcat(path, "/psinfo");

                            FILE* fp = fopen(path, "r");
                            char buf[255];
                            memset(buf, 0x00, 255);
                            fgets(buf, 255, (FILE*)fp);
                            fclose(fp);

                            // 获取ticks和进程状态
                            int j = 0;
                            for (i = 0; i < 4;) {
                                for (j = 0; j < 255; j++) {
                                    if (i >= 4) break;
                                    if (buf[j] == ' ') i++;
                                }
                            }
                            // 循环结束, buf[j]为进程的状态, 共有S, W, R三种状态.
                            int k = j + 1;
                            for (i = 0; i < 3;) {               // 循环结束后k指向ticks位置
                                for (k = j + 1; k < 255; k++) {
                                    if (i >= 3) break;
                                    if (buf[k] == ' ') i++;
                                }
                            }
                            long processTick = 0;
                            while (buf[k] != ' ') {
                                processTick = 10 * processTick + buff[k] - 48;
                                k++;
                            }
                            if(processTick>0)
                            {
                                totalTicks =totalTicks+ processTick;
                                if (buf[j] != 'R') {
                                    freeTicks = freeTicks+processTick;
                                }
                            }
                            
                        }else continue;
                    }else continue;
                }
        }
    }
    printf("CPU states: %.2lf%% used,\t%.2lf%% idle\n",
           (double)((totalTicks - freeTicks) * 100) / (double)totalTicks,
           (double)(freeTicks * 100) / (double)totalTicks);
    return;
}
