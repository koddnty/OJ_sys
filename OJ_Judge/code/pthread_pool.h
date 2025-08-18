#include <pthread.h>
#include <stdio.h>
#include <queue>
#include <semaphore.h>
#include <climits>


class Task{
public:
    void *(*function) (void*);
    void* arg;
};

class ThreadPool{
public:
    //初始化
    ThreadPool(int min = 3, int max = 10, int queueSize = 40);
    ~ThreadPool();
public:
    //线程退出
    void exitThread();
    //添加任务
    void* push(void *(*function) (void* arg), void* arg);
    //获取存活线程个数
    int getLiveNum();
    //获取busyNum
    int getBusyNum();
    //打印基本信息
    void printInfo();
    //阻塞等待
    void wait(int wait = INT_MAX );



private:
    static void* worker(void* arg);
    static void* manager(void* arg);


private:
    std::queue<Task> task_queue;           // 任务队列
    std::queue<pthread_t> tempID_queue;         //线程id缓存队列

    pthread_t managerID;                    //管理者线程ID
    pthread_t* threadID;                    //线程池ID数组
     
    int minNum;         //最小线程个数
    int maxNum;         //最大线程个数
    int busyNum;        //忙线程个数
    int liveNum;        //存活线程个数
    int exitNum;        //死亡线程个数
    int waitTime;       //等待时间(3倍,注意int范围)

    int shutdown;        //线程池是否销毁

    pthread_mutex_t mutexPool;          //线程池锁
    pthread_mutex_t mutexBusyNum;       //忙线程个数锁


    pthread_cond_t isEmpty;             //任务队列是否空了
};
