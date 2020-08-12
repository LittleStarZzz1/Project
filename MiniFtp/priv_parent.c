#include "priv_parent.h"
#include "privsock.h"
#include "sysutil.h"
#include "session.h"

//获取主动模式的数据连接套接字
static void privop_pasv_get_data_sock(session_t *sess); 

//获取是否处于被动模式的激活状态
static void privop_pasv_active(session_t *sess); 

//获取被动模式下的监听端口
static void privop_pasv_listen(session_t *sess);

//获取被动模式下的数据连接套接字
static void privop_pasv_accept(session_t *sess); 

/*int capset(cap_user_header_t hdrp,const cap_user_data_t datap)
{
    return syscall(__NR_capset, hdrp, datap);
}*/
//提升权限
static void minimize_privilege()
{    
    struct __user_cap_header_struct cap_header;
    struct __user_cap_data_struct cap_data;
    memset(&cap_header, 0, sizeof(cap_header));
    memset(&cap_data, 0, sizeof(cap_data));

    cap_header.version = _LINUX_CAPABILITY_VERSION_2;
    cap_header.pid = 0;

    unsigned int cap_mask = 0;
    cap_mask |= (1 << CAP_NET_BIND_SERVICE);

    cap_data.effective = cap_data.permitted = cap_mask;
    cap_data.inheritable = 0;//不继承

    capset(&cap_header, &cap_data);//用于设置权限能力
}

void handler_parent(session_t* sess)
{    
    //把root进程更名为 nobody进程                                                    
    struct passwd* pw = getpwnam("nobody");
    if (pw == NULL)                    
        ERR_EXIT("getpwnam error~~\n");
    if (setegid(pw->pw_gid) < 0)     
        ERR_EXIT("setgid error~~\n");
    if (seteuid(pw->pw_uid) < 0)      
        ERR_EXIT("seteuid error~~\n");      

    minimize_privilege();
    char cmd;
    while (1)
    {
        //不停地等待ftp进程的消息
        cmd = priv_sock_get_cmd(sess->parent_fd);
        switch(cmd)
        {
            case PRIV_SOCK_GET_DATA_SOCK:
                privop_pasv_get_data_sock(sess);
                break;
            case PRIV_SOCK_PASV_ACTIVE:
                privop_pasv_active(sess);
                break;
            case PRIV_SOCK_PASV_LISTEN:
                privop_pasv_listen(sess);
                break;
            case PRIV_SOCK_PASV_ACCEPT:
                privop_pasv_accept(sess);
                break;
        }
    }
}

static void privop_pasv_get_data_sock(session_t *sess)
{
    uint16_t port = (uint16_t)priv_sock_get_int(sess->parent_fd);
    char ip[16] = {0};
    priv_sock_recv_buf(sess->parent_fd, ip, sizeof(ip));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);//主机字节序转网络字节序
    address.sin_addr.s_addr = inet_addr(ip);

    int fd = tcp_client(20);
    if (fd == -1)
    {
        priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_BAD);
        return;
    }
    if (connect(fd, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        close(fd);
        priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_BAD);
        return;
    }
    priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_OK);
    priv_sock_send_fd(sess->parent_fd, fd);
    close(fd);//将描述符发送给ftp服务进程之后,nobody进程用不到这个描述符了,直接关闭
}

static void privop_pasv_active(session_t *sess)
{
    int active;
    if (sess->pasv_listen_fd != -1)
        active = 1;
    else
        active = 0;
    priv_sock_send_int(sess->parent_fd, active);
}

static void privop_pasv_listen(session_t *sess)
{
    //char* ip = "192.168.188.131";//暂且写死
    char ip[16] = {0};
    getLocalip(ip);

    sess->pasv_listen_fd = tcp_server(ip, 0);//传端口0表示绑定临时端口

    struct sockaddr_in address;
    socklen_t addrlen = sizeof(struct sockaddr);
    if (getsockname(sess->pasv_listen_fd, (struct sockaddr*)&address, &addrlen) < 0)
        ERR_EXIT("getsockname error~~\n");

    uint16_t port = ntohs(address.sin_port);
    priv_sock_send_int(sess->parent_fd, (int)port);
}

static void privop_pasv_accept(session_t *sess)
{
    int fd = accept(sess->pasv_listen_fd, 0, 0);
    close(sess->pasv_listen_fd);
    sess->pasv_listen_fd = -1;

    if (fd == -1)
    {
        priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_BAD);
        return;
    }
    priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_OK);
    priv_sock_send_fd(sess->parent_fd, fd);
    close(fd);
}

