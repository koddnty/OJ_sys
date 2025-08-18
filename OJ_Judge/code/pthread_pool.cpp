#include "pthread_pool.h"
#include <unistd.h>

ThreadPool::ThreadPool(int min, int max, int queueSize){
    do{
        this->waitTime = INT_MAX;
        this->minNum = min;
        this->maxNum = max;
        this->liveNum = 0;
        this->busyNum = 0;
        this->exitNum = 0;
        this->shutdown = 0;

        //创建锁与信号量
        if(pthread_mutex_init(&(this->mutexBusyNum), NULL) != 0 ||
            pthread_mutex_init(&(this->mutexPool), NULL) || 
            pthread_cond_init(&(this->isEmpty), NULL))
        {
            fprintf(stderr, "创建锁或sem失败\n");
            pthread_mutex_destroy(&(this->mutexBusyNum));
            pthread_mutex_destroy(&(this->mutexPool));
            pthread_cond_destroy(&(this->isEmpty));  
            break;
        }


        if(pthread_create(&(this->managerID), NULL, manager, this) != 0){
            break;
        }
        // pthread_join(managerID, NULL);
        this->threadID = new pthread_t[max] ();
        // for(int i = 0; i < min; i++){
        //     if(pthread_create(&(this->threadID[i]), NULL, this->worker, this) != 0){
        //         break;
        //     }
        // }
        return ;
    }while(0);
    if(this->threadID != NULL){
        delete[] this->threadID;
    }
}
ThreadPool::~ThreadPool(){
    //设置销毁指令
    this->shutdown = 1;
    //销毁管理线程
    //pthread_join(this->managerID, NULL);
    //销毁工作线程
    while(this->liveNum){
        pthread_mutex_lock(&this->mutexPool);
        this->exitNum++;
        pthread_mutex_unlock(&this->mutexPool);
        fprintf(stderr, "发送销毁信号");
        pthread_cond_signal(&this->isEmpty);
        usleep(100000);
    }
    //销毁堆
    delete[] this->threadID;
    //销毁锁
    pthread_mutex_destroy(&this->mutexPool);
    pthread_mutex_destroy(&this->mutexBusyNum);
    pthread_cond_destroy (&this->isEmpty);
}


//工作线程
void* ThreadPool::worker(void* arg){
    ThreadPool* pool = (ThreadPool*)arg;
    while(1){
        pthread_mutex_lock(&(pool->mutexPool));
        while(!(pool->shutdown) && (pool->task_queue).empty()){
            pthread_cond_wait(&pool->isEmpty, &pool->mutexPool);
            if(pool->exitNum > 0){
                pool->exitNum--;
                pool->liveNum--;
                pthread_mutex_unlock(&(pool->mutexPool));
                pool->exitThread();
            }
        }
        if(pool->shutdown){
            pthread_mutex_unlock(&(pool->mutexPool));
            pthread_exit(NULL);
        }
        else{
            Task task = (pool->task_queue).front();
            (pool->task_queue).pop();
            pthread_mutex_unlock(&(pool->mutexPool));
            pthread_cond_signal(&pool->isEmpty);
            pthread_mutex_lock(&(pool->mutexBusyNum));
            pool->busyNum++;
            pthread_mutex_unlock(&(pool->mutexBusyNum));
            (*task.function)(task.arg);
            pthread_mutex_lock(&pool->mutexBusyNum);
            pool->busyNum--;
            pthread_mutex_unlock(&pool->mutexBusyNum);
        }
    }
    pthread_exit(NULL);
}
//管理者线程
void* ThreadPool::manager(void* arg){
    int time = 0;
    ThreadPool* pool = (ThreadPool*)arg;
    while(!pool->shutdown){
        //创建线程
        if(pool->busyNum < pool->task_queue.size() && pool->liveNum < pool->maxNum){
            pthread_mutex_lock(&pool->mutexPool);
            for(int i = 0; i < pool->maxNum; i++){
                if(pool->threadID[i] == 0){
                    if(pthread_create(pool->threadID + i, NULL, pool->worker, pool)){
                        perror("pthread_create");
                        break;
                    }
                    // pthread_join(pool->threadID[i], NULL);
                    pthread_detach(pool->threadID[i]);
                    fprintf(stderr, "新增线程,liveNum:%d\n", pool->liveNum + 1);
                    (pool->liveNum)++;
                }
            }
            pthread_mutex_unlock(&pool->mutexPool);
        }
        //销毁线程
        if(pool->busyNum * 2 < pool->liveNum && pool->liveNum > pool->minNum){
            while(pool->busyNum * 2 < pool->liveNum && pool->liveNum > pool->minNum){
                pthread_mutex_lock(&pool->mutexPool);
                pool->exitNum++;
                pthread_mutex_unlock(&pool->mutexPool);
                fprintf(stderr, "发送销毁信号");
                pthread_cond_signal(&pool->isEmpty);
                usleep(1000);
            }
        }
        sleep(3);
        //线程池关闭设置
        if(pool->busyNum == 0){
            time++;
            time %= INT_MAX - 1;
        }
        else{
            time = 0;
        }
        if(time >= pool->waitTime){
            while(pool->liveNum){
                pthread_mutex_lock(&pool->mutexPool);
                pool->exitNum++;
                pthread_mutex_unlock(&pool->mutexPool);
                fprintf(stderr, "发送销毁信号");
                pthread_cond_signal(&pool->isEmpty);
                usleep(1000);
            }
            break;
        }
    }
    return NULL;
}
//添加任务
void* ThreadPool::push(void *(*func) (void*) , void* arg){
    pthread_mutex_lock(&this->mutexPool);
    Task task;
    task.function = func;
    task.arg = arg;
    task_queue.push(task);
    pthread_mutex_unlock(&this->mutexPool);
    pthread_cond_signal(&this->isEmpty);
    return NULL;
}
//阻塞等待
void ThreadPool::wait(int wait){
    this->waitTime = wait;
    pthread_join(this->managerID, NULL);
}
//线程退出
void ThreadPool::exitThread(){
    pthread_t tid = pthread_self();
    for(int i = 0; i < this->maxNum; i++){
        if(this->threadID[i] == tid){
            fprintf(stderr, "pthread %ld exiting...\n", threadID[i]);
            threadID[i] = 0;
            break;
        }
    }
    pthread_exit(NULL);
}


//获取线程池信息
int ThreadPool::getBusyNum(){
    //未加锁，可能不准确
    return this->busyNum;
}
int ThreadPool::getLiveNum(){
    return this->liveNum;
}
void ThreadPool::printInfo(){
    fprintf(stderr, "minNum  : %d\n", minNum);
    fprintf(stderr, "maxNum  : %d\n", maxNum);
    fprintf(stderr, "busyNum : %d\n", getBusyNum());
    fprintf(stderr, "liveNum : %d\n", getLiveNum());
}