
#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <pwd.h>
#include <grp.h>
#include <getopt.h>
//终端颜色
#define COLOR_RESET   "\033[0m"
#define COLOR_DIR     "\033[1;34m"
#define COLOR_EXEC    "\033[1;32m"
#define COLOR_LINK    "\033[1;36m"
#define COLOR_DEFAULT "\033[0m"
//文件信息
typedef struct
{
    char *name;          
    char *path;          
    const char *color;   
    struct stat st;      
} File;

typedef struct
{
    bool a_flag;         
    bool l_flag;         
    bool R_flag;         
    bool t_flag;         
    bool i_flag;         
    bool s_flag;         
} Choose;

static Choose choose = {0};

static void parse_args(int argc,char *argv[])
{
    int opt;

    while((opt=getopt(argc,argv,"alRtis"))!=-1)
    {
        switch(opt)
        {
            case'a':
            choose.a_flag=true;
            break;

            case'l':
            choose.l_flag=true;
            break;

            case'R':
            choose.R_flag=true;
            break;

            case't':
            choose.t_flag=true;
            break;

            case 'i':
            choose.i_flag = true;
            break;

            case's':
            choose.s_flag=true;
            break;

            default:
            fprintf(stderr, "未知选项\n");
            exit(EXIT_FAILURE);
        }
    }
}
//根据文件类型决定显示颜色
static const char *get_file_color(const struct stat *st)
{
    if(S_ISDIR(st->st_mode))//如果是目录
    {
        return COLOR_DIR;//返回颜色字符串
    }
    else if(S_ISLNK(st->st_mode))//如果是链接
    {
        return COLOR_LINK;
    }
    else if(S_ISREG(st->st_mode)&&(st->st_mode & S_IXUSR))//如果是常规文件
    {
        return COLOR_EXEC;
    }
    else//其他的
    {
        return COLOR_DEFAULT;
    }
}
//将文件类型和权限转换为字符串
static void get_permission_str(mode_t mode,char *str)
{
    //判断文件类型
    if(S_ISDIR(mode))
    str[0]='d';
    else if(S_ISLNK(mode))
    str[0]='l';
    else if(S_ISREG(mode))
    str[0]='-';
    else
    str[0]='?';

    //所有者权限
    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';

    //所属组权限
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';

    //其他用户权限
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';

    str[10] = '\0';
}

//根据文件的UID获取文件所有者的用户名
static void get_user_name(uid_t uid,char *name,size_t size)
{
    struct passwd *pwd=getpwuid(uid);//根据传入的UID查询用户信息

    if(pwd!=NULL)
    {
        snprintf(name,size,"%s",pwd->pw_name);
    }
    else{
        snprintf(name,size,"%u",(unsigned int)uid);
    }
}

//根据
int main(int argc, char *argv[])
{
    parse_args(argc,argv);

    printf("a_flag=%d\n",choose.a_flag);
    printf("l_flag=%d\n",choose.l_flag);
    printf("R_flag=%d\n",choose.R_flag);
    printf("t_flag=%d\n",choose.t_flag);
    printf("i_flag=%d\n",choose.i_flag);
    printf("s_flag=%d\n",choose.s_flag);//暂时用来验证参数是否正确
    return 0;
}
