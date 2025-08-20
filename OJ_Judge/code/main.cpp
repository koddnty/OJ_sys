#include "linuxHeader.h"
#include "user_info.h"
#include "tool.h"
#include "judge.h"
#include <iostream>
#include <iomanip>

//评判沙箱锁
pthread_mutex_t* lock_list = new pthread_mutex_t[MAX_ISOLATE_MUTEX_LOCK];
//评判任务线程池
ThreadPool* isolate_pool = new ThreadPool(2, 5, 40);



using namespace std;
using nlohmann::json;

jsonManager jsonManager::s_;


static WFFacilities::WaitGroup wait_M(1);

void Free_func();

void signalHander(int num){
    wait_M.done();
    Free_func();
    fprintf(stderr, "\n>>>>程序停止<<<<\n");
}

int main(int argc, char* argv[]){
    //初始化所有隔离全局锁
    for(int i = 0; i < MAX_ISOLATE_MUTEX_LOCK; i++){
        pthread_mutex_init(&lock_list[i], nullptr);
    }

    signal(SIGINT, signalHander);
    wfrest::HttpServer server;
    std::string path = "";
    if(sizeof(argv[1]) == 0){
        fprintf(stderr, "请指定配置文件\n");
        return 0;
    }
    else{
        path  =  argv[1];
    }
    if(path == ""){
        std::cout << "空路径" << std::endl;
        return 0;
    }
    try{
        jsonManager::getManager().jsonLoad(path);
    }catch(const  std::exception& _e){
        std::cout << "配置文件解析失败" << std::endl;
        return 0;
    }
    jsonManager::getManager().print();

    //逻辑业务实现
    //用户注册
    server.POST("/user/register", UserInfo::userRegist);
    //用户登录
    server.POST("/user/login", UserInfo::userLogin);
    //上传题目
    server.POST("/add/problem", Judge::add_problem);
    //题目评判
    //上传用户答案
    server.POST("/problem/user_solve", Judge::runCpp);
    //上传测试输入
    server.POST("/problem/upload_testcase_in", Judge::add_test_in);
    //上传测试输出
    server.POST("/problem/upload_testcase_out", Judge::add_test_out);
    //获取运行状态
    server.GET("/problem/get_slove_info", Judge::runState);
    //获取题目列表
    server.GET("/problem/get_problem_list", Judge::get_problem_list);




    if(server.track().start(7778) == 0){
        fprintf(stderr, "启动成功\n");
        wait_M.wait();
        server.stop();
    }
    else{
        fprintf(stderr, "启动失败\n");
        Free_func();
    }
    return 0;
}



//终止释放函数
void Free_func(){
    for(int i = 0; i < MAX_ISOLATE_MUTEX_LOCK; i++){
        pthread_mutex_destroy(lock_list + i);
    }
    fprintf(stderr, "释放函数完成\n");
}

