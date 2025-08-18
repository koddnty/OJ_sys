#include "linuxHeader.h"
#include "user_info.h"
#include "tool.h"
#include <iostream>
#include <iomanip>

using namespace std;
using nlohmann::json;

jsonManager jsonManager::s_;
static WFFacilities::WaitGroup wait(1);

void signalHander(int num){
    wait.done();
    fprintf(stderr, "\n>>>>程序停止<<<<\n");
}

int main(int argc, char* argv[]){
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


    if(server.track().start(7778) == 0){
        fprintf(stderr, "启动成功\n");
        wait.wait();
        server.stop();
    }
    else{
        fprintf(stderr, "启动失败\n");
    }
    return 0;
}