#ifndef _COROUTINE_H //_COROUTINE_H
#define _COROUTINE_H //_COROUTINE_H

#include <ucontext.h>
//协程栈大小
#define STACK_SIZE (1024*64)
//最大协程数
#define COROUTINE_SIZE (1024)

enum State
{
    //初始化协程状态--就绪态--ready
    READY, 
    //协程开始执行的状态--运行态--RUNNING
    RUNNING,
    //协程主动放弃cpu
    SUSPEND,
    //协程执行完毕
    DEAD
};

struct schedule;

//协程
typedef struct coroutine
{
    //回调函数
    void* (*_call_back)(struct schedule* s, void* args);
    //回调函数参数
    void* _args;
    //协程的上下文信息
    ucontext_t _ctx;
    //协程栈
    char _stack[STACK_SIZE];
    //协程状态
    enum State _state;
}coroutine_t;

//协程调度器
typedef struct schedule
{
    //存储指向每个协程的指针数组
    coroutine_t** _coroutines;    
    //当前处于运行态的协程在数组中的下标
    int _current_id;
    //数组中所存储的所有线程的最大下标
    int _maxid;
    //主控流程的上下文数据
    ucontext_t _main_ctx;
}schedule_t;

//创建协程调度器
schedule_t* schedule_create();

//创建协程
int coroutine_create(schedule_t* s, void* (*call_back)(schedule_t* s, void* args), void* args);

//协程执行函数
static void* _main_func(schedule_t* s); 


//获取协程当前状态
enum State get_coState(schedule_t* s, int id);

//启动协程
void coroutine_running(schedule_t* s, int id);

//让出cpu
void coroutine_yield(schedule_t* s);

//恢复CPU
void coroutine_resume(schedule_t* s, int id);

//删除协程
void coroutine_delete(schedule_t* s, int id);

//释放协程调度器
void schedule_delete(schedule_t* s);

//判断调度器中所有协程是否允许完毕
int schedule_finish(schedule_t* s);

#endif //_COROUTINE_H
