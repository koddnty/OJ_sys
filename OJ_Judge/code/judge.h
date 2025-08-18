#include "linuxHeader.h"
#include "tool.h"
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wordexp.h>

//类system函数
int system_spawn_cmd(const char *cmd);
//编译运行c++程序
int cpp_runner(std::string& path);

class Judge{
public:
    //添加题目
    static void add(const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series);
    //评判cpp
    static void runCpp (const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series);
    //获取题目列表
    static void get_problem_list(const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series);
};