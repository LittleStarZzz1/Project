#include "ftp_proto.h"
#include "ftp_codes.h"
#include "str.h"

//ftp服务进程

// --------------
// |命令映射机制|
// --------------

typedef struct ftpcmd
{
   const char* cmd;//命令
   void (*cmd_handler)(session_t* sess);//命令对应的回调函数
}ftpcmd_t;

static void do_user(session_t* sess);
static void do_pass(session_t* sess);
static void do_syst(session_t* sess);

//命令映射表
static ftpcmd_t ctrl_cmds[] =
{
    {"USER", do_user},
    {"PASS", do_pass},
    {"SYST", do_syst}
};

//////////////////////////////////////////////////////////////////////


//封装回应函数
void ftp_reply(session_t* sess, int code, const char* text)
{
    char buf[MAX_BUFFER_SIZE] = {0};
    sprintf(buf, "%d %s\r\n", code, text);
    send(sess->ctrl_fd, buf, strlen(buf), 0);
}

void handler_child(session_t* sess)
{
    //send(sess->ctrl_fd, "220 (MiniFtp 1.0)\r\n",strlen("220 (MiniFtp 1.0))\r\n"), 0);
    ftp_reply(sess, FTP_GREET, "(test MiniFtp 1.0)");

    int ret;
    while (1)
    {
        //不停地等待客户端的命令并进行处理
        memset(sess->cmd_line, 0, MAX_COMMAND_SIZE);
        memset(sess->cmd, 0, MAX_COMMAND);
        memset(sess->arg, 0, MAX_ARG);

        ret = recv(sess->ctrl_fd, sess->cmd_line, MAX_COMMAND_SIZE, 0);
        if (ret == -1)
            ERR_EXIT("recv error~~\n");
        else if (ret == 0)
        {
            exit(EXIT_SUCCESS);
        }
        // printf("recv success~~\n");
        //接下来对命令进行解析
        str_trim_crlf(sess->cmd_line);
        //printf("cmd_line : %s\n", sess->cmd_line); 

        str_split(sess->cmd_line, sess->cmd, sess->arg, ' ');
        //printf("cmd : %s\n", sess->cmd);
        //printf("arg : %s\n", sess->arg);

        int table_size = sizeof(ctrl_cmds) / sizeof(ftpcmd_t);
        int i;

        for (i = 0; i < table_size; ++i)
        {
            //该命令存在命令映射表当中
            if (strcmp(sess->cmd, ctrl_cmds[i].cmd) == 0)
            {
                if (ctrl_cmds[i].cmd_handler != NULL)//回到函数不为空
                {
                    ctrl_cmds[i].cmd_handler(sess);
                }
                else//回调函数为空,表示该命令的回调函数未实现
                {
                    ftp_reply(sess, FTP_COMMANDNOTIMPL, "Unimplement command.");
                }
                break;
            }
        }

        if (i >= table_size)
        {
            //该命令不存在命令映射表当中
            ftp_reply(sess, FTP_BADCMD, "Unknown command.");
        }
    }
}


//对应命令的回调函数
static void do_user(session_t* sess)
{
    struct passwd* pwd = getpwnam(sess->arg);

    if (pwd != NULL)
    {
        sess->uid = pwd->pw_uid;
    }
    ftp_reply(sess, FTP_GIVEPWORD, "Please specify the password.");
}

static void do_pass(session_t* sess)
{

}

static void do_syst(session_t* sess)
{

}






