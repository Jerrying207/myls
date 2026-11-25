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
    bool r_flag;      
    bool t_flag;         
    bool i_flag;         
    bool s_flag;         
} Choose;

static Choose choose = {0};//初始化

//读取命令行
static void parse_args(int argc,char *argv[])
{
    int opt;

    while((opt=getopt(argc,argv,"alRrtis"))!=-1)
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

            case'r':
            choose.r_flag=true;
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
    else if(S_ISCHR(mode))
    str[0]='c';
    else if(S_ISBLK(mode))
    str[0]='b';
    else if(S_ISFIFO(mode))
    str[0]='p';
    else if(S_ISSOCK(mode))
    str[0]='s';
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

    if(mode&S_ISUID)
    {
        str[3]=(mode&S_IXUSR)?'s':'S';
    }
    if(mode&S_ISGID)
    {
        str[6]=(mode&S_IXGRP)?'s':'S';
    }
    if(mode&S_ISVTX)
    {
        str[9]=(mode&S_IXOTH)?'t':'T';
    }

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

//根据文件的GID获取文件所有者的组名
static void get_group_name(gid_t gid,char *name,size_t size)
{
    struct group *grp=getgrgid(gid);
    if(grp!=NULL)
    {
        snprintf(name,size,"%s",grp->gr_name);
    }
    else{
        snprintf(name,size,"%lu",(unsigned long)gid);
    }
}

//文件修改时间比较
static int cmp_time(const void *a,const void *b)
{
    //把传进来的通用指针转换成 File 结构体指针，这样就能访问文件信息了
    const File *file_a = (const File *)a;
    const File *file_b = (const File *)b;

    if(file_a->st.st_mtime>file_b->st.st_mtime)
    return -1;
    else if(file_a->st.st_mtime<file_b->st.st_mtime)
    return 1;
    else
    return strcmp(file_a->name,file_b->name);
}

static int cmp_name(const void *a,const void *b)
{
    const File *file_a=(const File *)a;
    const File *file_b=(const File *)b;

    return strcmp(file_a->name,file_b->name);
}
//释放单个文件的信息
static void free_file(File *file)
{
    if(file==NULL)
    return;

    free(file->name);
    free(file->path);
    file->path=NULL;
    file->name=NULL;
}

//释放整个文件数组的信息
static void free_files(File *files,size_t count)
{
    if(files==NULL)
    return;

    for(size_t i=0;i<count;i++)
    {
        free_file(&files[i]);
    }
    free(files);
}

//格式化输出文件信息
static void print(const File *file,int owner_width,int group_width,int size_width)
{
    char permission[11];
    char owner[64];
    char group[64];
    char time_str[32];

    struct tm tm_buf;
    get_permission_str(file->st.st_mode,permission);

    get_user_name(file->st.st_uid,owner,sizeof(owner));
    get_group_name(file->st.st_gid,group,sizeof(group));

    localtime_r(&file->st.st_mtime,&tm_buf);
    strftime(time_str,sizeof(time_str),"%b %e %H:%M",&tm_buf);

    if(choose.i_flag)
    printf("%llu ",(unsigned long long)file->st.st_ino);

    if(choose.s_flag)
    printf("%llu ",(unsigned long long)((file->st.st_blocks+1)/2));

    if(choose.l_flag)
    {
        printf("%s %2lu %-*s %-*s %*lld %s",
        permission,
        (unsigned long)file->st.st_nlink,
        owner_width,owner,
        group_width, group,
        size_width,(long long)file->st.st_size,
        time_str);
    }
    printf(" %s%s%s",file->color,file->name,COLOR_RESET);
    if(choose.l_flag&&S_ISLNK(file->st.st_mode))
    {
        char target[4096];

        ssize_t len=readlink(file->path,target,sizeof(target)-1);

        if(len==-1)
        {
            perror("readlink");
        }
        else{
            target[len]='\0';
            printf("->%s",target);
        }
    }
    putchar('\n');

}
//遍历目录
static void process_dir(const char *path)
{
    DIR *dir=opendir(path);
    //先初始化
    if(dir==NULL)
    {
        perror(path);
        return;
    }
    File *files=NULL;
    size_t count=0;
    size_t capacity=0;

    struct dirent *entry;
    while(1)
    {
        errno=0;
        entry=readdir(dir);

        if(entry==NULL)
        {
            if(errno!=0)
            {
                perror("readdir");
            }
        break;
        }

        if(!choose.a_flag&&entry->d_name[0]=='.')
        {
            continue;
        }
        
        char full_path[4096];
        snprintf(full_path,sizeof(full_path),"%s/%s",path,entry->d_name);
        struct stat st;

        if(lstat(full_path,&st)==-1)
        {
            perror(full_path);
            continue;
        }
        if(count==capacity)
        {
            size_t new_capacity=capacity==0?16:capacity*2;

            File *temp=realloc(files,new_capacity*sizeof(File));

            if(temp==NULL)
            {
                perror("realloc");
                free_files(files,count);
                closedir(dir);
                return;
            }
            files=temp;
            capacity=new_capacity;
        }
        
        files[count].name=strdup(entry->d_name);
        files[count].path=strdup(full_path);

        if(files[count].name==NULL||files[count].path==NULL)
        {
            perror("strdup");
            free(files[count].name);
            free(files[count].path);
            free_files(files,count);
            closedir(dir);
            return;
        }

        files[count].st=st;
        files[count].color=get_file_color(&st);
        count++;
    }
    closedir(dir);

    if(count>1)
    {
        if(choose.t_flag)
        qsort(files,count,sizeof(File),cmp_time);
        else
        qsort(files,count,sizeof(File),cmp_name);
    }

    if(choose.r_flag)
    {
    for(size_t i=0;i<count/2;i++)
        {
            File temp=files[i];
            files[i]=files[count-1-i];
            files[count-1-i]=temp;
        }
    }

    int owner_width=8;
    int group_width=8;
    int size_width=8;

    for(size_t i=0;i<count;i++)
    {
        char owner[64];
        char group[64];
        char size_str[32];
        get_user_name(files[i].st.st_uid,owner,sizeof(owner));
        get_group_name(files[i].st.st_gid,group,sizeof(group));

        snprintf(size_str,sizeof(size_str),"%lld",(long long)files[i].st.st_size);
        
        int len;
        len=strlen(owner);
        if(len>owner_width)
        {
            owner_width=len;
        }
        len=strlen(group);
        if(len>group_width)
        {
            group_width=len;
        }
        len=strlen(size_str);
        if(len>size_width)
        {
            size_width=len;
        }
    }

    for(size_t i=0;i<count;i++)
    {
        print(&files[i],owner_width,group_width,size_width);
    }
    if(choose.R_flag)
    {
        for(size_t i=0;i<count;i++)
        {
            if(S_ISDIR(files[i].st.st_mode)&&strcmp(files[i].name,".")!=0&&strcmp(files[i].name,"..")!=0)
            {
                printf("\n%s:\n",files[i].path);
                process_dir(files[i].path);
            }
        }
    }
    free_files(files,count);
}

//判断路径是目录还是文件
static void process_path(const char *path)
{
    struct stat st;

    if(lstat(path,&st)==-1)
    {
        perror(path);
        return;
    }
    if(S_ISDIR(st.st_mode))
    {
        process_dir(path);
    }
    else{
        File file;

        file.name=(char*)path;
        file.path=(char*)path;
        file.st=st;
        file.color=get_file_color(&st);

        print(&file,8,8,8);
    }
}

int main(int argc, char *argv[])
{
    parse_args(argc,argv);
    if(optind<argc)
    {
        int path_count=argc-optind;

        for(int i=optind;i<argc;i++)
        {
            struct stat st;
            if(path_count>1&&
            lstat(argv[i],&st)==0&&
            S_ISDIR(st.st_mode))
            {
                if(i>optind)
                putchar('\n');

                printf("%s:\n",argv[i]);
            }
            process_path(argv[i]);
        }
    }
    else{
        process_path(".");
    }
    return 0;
}
