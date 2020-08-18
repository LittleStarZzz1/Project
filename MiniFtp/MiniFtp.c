#include "common.h"
#include "sysutil.h"
#include "session.h"
#include "parseconf.h"
#include "tunable.h"
#include "ftp_codes.h"
#include "ftp_proto.h"
#include "hash.h"

void ParseConf_Test()
{
    parseconf_load_file("MiniFtp.conf");

    printf("tunable_pasv_enable = %d\n", tunable_pasv_enable);
    printf("tunable_port_enable = %d\n", tunable_port_enable);
    printf("tunable_listen_port = %d\n", tunable_listen_port);
    printf("tunable_max_clients = %d\n", tunable_max_clients);
    printf("tunable_max_per_ip = %d\n", tunable_max_per_ip);
    printf("tunable_accept_timeout = %d\n", tunable_accept_timeout);
    printf("tunable_connect_timeout = %d\n", tunable_connect_timeout);
    printf("tunable_idle_session_timeout = %d\n", tunable_idle_session_timeout);
    printf("tunable_data_connection_timeout = %d\n", tunable_data_connection_timeout);
    printf("tunable_loacl_umask = %d\n", tunable_local_umask);
    printf("tunable_upload_max_rate = %d\n", tunable_upload_max_rate);
    printf("tunable_download_mas_rate = %d\n", tunable_download_max_rate);
    printf("tunable_listen_address = %s\n", tunable_listen_address);
}


//最大连接数限制
static unsigned int s_children;

//ip-每ip对应连接数映射关系哈希表
static hash_t *s_ip_count_hash;
//计算每ip对应的连接数
unsigned int handle_ip_count(void *ip);

//子进程退出之后的信号处理回调函数
void handle_sigchld(int sig);

//每ip连接数限制
static hash_t *s_pid_ip_hash;//pid-ip映射关系哈希表
//将ip对应的连接数减1
void drop_ip_count(void *ip);


static void check_limit(session_t* sess);

int main(int argc, char* argv)
{
    parseconf_load_file("MiniFtp.conf");
    
    //char ip[16] = {0};
    //getLocalip(ip);
    //printf("ip = %s\n", ip);

    //配置文件解析的测试
    //ParseConf_Test();

    //权限提升, 以root用户打开Miniftp
    if (getuid() != 0)
    {
        printf("MiniFtp must be started as root.\n");
        exit(EXIT_FAILURE);
    }
    //初始化会话信息
    session_t sess = 
    {
        //控制连接
        -1, -1, "", "", "",
        
        //数据连接
        NULL, -1, -1,

        // ftp协议状态
        0, NULL, 0, 0, 0
        
        //父子进程通道
        -1, -1,

        //限速
        0, 0, 0, 0
    };
    
    sess.upload_max_rate = tunable_upload_max_rate;
    sess.download_max_rate = tunable_download_max_rate;


    //创建监听套接字
    //int listen_fd = tcp_server(tunable_listen_address, tunable_listen_port);
    int listen_fd = tcp_server(tunable_listen_address, tunable_listen_port);

    signal(SIGCHLD, handle_sigchld);

    int sock_Conn;
    struct sockaddr_in addr_Cli;
    socklen_t addrlen;
    while (1)
    {
        //不断循环获取客户端的连接
        sock_Conn = accept(listen_fd, (struct sockaddr*)&addr_Cli, &addrlen);

        ++s_children;
        sess.num_clients = s_children;

        if (sock_Conn < 0)
            ERR_EXIT("accept error~~~\n");

        //创建子进程
        pid_t pid = fork();

        if (pid == -1)
            ERR_EXIT("fork error~~~\n");

        if (pid == 0)//子进程
        {
            close(listen_fd);

            //设置会话
            sess.ctrl_fd = sock_Conn;

            //进行连接数检查限制
            check_limit(&sess);
            
            session_begin(&sess);

            exit(EXIT_SUCCESS);
        }
        else//父进程
        {
            close(sock_Conn);
        }
    }
    //关闭套接字
    close(listen_fd);

    return 0;
}

static void check_limit(session_t* sess)
{
    if (tunable_max_clients != 0 && sess->num_clients > tunable_max_clients)
    {
        // 421 There are too many connected users, please try later
        ftp_reply(sess, FTP_TOO_MANY_USERS, "There are too many connected users, please try later");
        exit(EXIT_FAILURE);
    }
    if (tunable_max_per_ip != 0 && sess->num_per_ip > tunable_max_per_ip)
    {
        // 421 There are too many connections from your internet address
    }
}

void handle_sigchld(int sig)
{
    pid_t pid;
    while ((waitpid(-1, NULL, WNOHANG)) > 0)
    {
        --s_children;
        unsigned int *ip = hash_lookup_entry(s_pid_ip_hash, &pid, sizeof(pid));

        if (ip == NULL)
        {
            continue;
        }
        drop_ip_count(ip);
        hash_free_entry(s_pid_ip_hash, &pid, sizeof(pid));
    }
}

//哈希函数
unsigned int hash_func(unsigned int buckets, void *key)
{
    return (*(unsigned int*)key % buckets);
}








