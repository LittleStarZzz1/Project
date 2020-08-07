#include "priv_parent.h"
#include "privsock.h"

//获取被动模式的数据连接套接字
static void privop_pasv_get_data_sock(session_t *sess); 

//获取是否处于被动模式的激活状态
static void privop_pasv_active(session_t *sess); 

//获取被动模式下的监听端口
static void privop_pasv_listen(session_t *sess);

//获取被动模式下的数据连接套接字
static void privop_pasv_accept(session_t *sess); 



void handler_parent(session_t* sess)
{
    char cmd;
    while (1)
    {
        //不停地等待ftp进程的消息
        cmd = priv_sock_get_cmd(sess->child_fd);
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
{}

static void privop_pasv_active(session_t *sess)
{}

static void privop_pasv_listen(session_t *sess)
{}

static void privop_pasv_accept(session_t *sess)
{}

