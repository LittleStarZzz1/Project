#ifndef _TUN_ABLE_H
#define _TUN_ABLE_H

#include "common.h"

//全局变量声明
extern int tunable_pasv_enable;                       //是否开启被动模式
extern int tunable_port_enable;                       //是否开启主动模式

extern unsigned int tunable_listen_port;              //FTP服务器端口
extern unsigned int tunable_max_clients;              //最大连接数
extern unsigned int tunable_max_per_ip;               //每ip最大连接数
extern unsigned int tunable_accept_timeout;           //Accept超时间
extern unsigned int tunable_connect_timeout;          //Connect超时间
extern unsigned int tunable_idle_session_timeout;     //控制连接超时时间
extern unsigned int tunable_data_connection_timeout;  //数据连接超时时间
extern unsigned int tunable_loacl_umask;              //掩码
extern unsigned int tunable_upload_max_rate;          //最大上传速度 10M
extern unsigned int tunable_download_mas_rate;        //最大下载速度 10M
extern const char *tunable_listen_address;            //FTP服务器IP地址






#endif //_TUN_ABLE_H
