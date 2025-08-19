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
//编译运行c++程序,输入路径，返回结果。
int cpp_runner(std::string path, std::string file_name);

class Judge{
public:
    //添加题目
    static void add_problem(const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series);
    //添加题目测试用例_输入
    static void add_test_in(const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series);
    //添加题目测试用例_输出
    static void add_test_out(const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series);
    //评判cpp
    //上传用户答案
    static void runCpp (const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series);
    //获取运行状态
    static void runState(const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series);
    //获取题目列表
    static void get_problem_list(const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series);
};