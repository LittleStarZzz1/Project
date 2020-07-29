#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "Coroutine.h"

void* func1(schedule_t* s, void* args)
{
    printf("func1 start running~~\n");
    coroutine_yield(s);
    int res = *((int*)args);
    printf("func : data : %d\n", res);
}

void* func2(schedule_t* s, void* args)
{
    printf("fun2 start running~~~\n");
    coroutine_yield(s);
    int res = *((int*)args);
    printf("fun2 data : %d\n", res);
}

int main()
{
    schedule_t* s = schedule_create();//创建协程调度器

    int* a = (int*)malloc(sizeof(int));
    *a = 1;

    int* b = (int*)malloc(sizeof(int));
    *b = 2;

    int id_1 = coroutine_create(s, func1, (void*)a);
    int id_2 = coroutine_create(s, func2, (void*)b);

    coroutine_running(s, id_1);
    coroutine_running(s, id_2);

    while (!schedule_finish(s))
    {
        coroutine_resume(s, id_1);
        coroutine_resume(s, id_2);
    }

    

    schedule_delete(s);
    free(a);
    free(b);

    return 0;
}
