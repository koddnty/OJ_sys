#include "judge.h"
#include <pthread.h>
#include <sw/redis++/redis++.h>
#include <sys/stat.h>
#include <sw/redis++/redis++.h>
#include <vector>
#include <string>


// 直接设置 Redis 字段值
void redis_set_field(const std::string key, const std::string value) {
    try {
        sw::redis::Redis redis(jsonManager::getManager().JsonGet()["redis"]["redisIP"]); // 根据你的配置修改库号
        redis.set(key, value);
    } catch (const sw::redis::Error &err) {
        std::cerr << "Redis set error: " << err.what() << std::endl;
    }
}
// 执行 INCR 命令，返回自增后的值
long redis_incr(const std::string key) {
    try {
        sw::redis::Redis redis(jsonManager::getManager().JsonGet()["redis"]["redisIP"]);
        return redis.incr(key);
    } catch (const sw::redis::Error &err) {
        std::cerr << "Redis INCR error: " << err.what() << std::endl;
        return -1;
    }
}
// 设置 key 的过期时间（单位：秒），返回 true 表示成功
bool redis_expire(const std::string key, int seconds) {
    try {
        sw::redis::Redis redis(jsonManager::getManager().JsonGet()["redis"]["redisIP"]);
        return redis.expire(key, seconds);
    } catch (const sw::redis::Error &err) {
        std::cerr << "Redis EXPIRE error: " << err.what() << std::endl;
        return false;
    }
}


int system_spawn_cmd(const char *cmd) {
    wordexp_t p;
    if (wordexp(cmd, &p, 0) != 0) {
        fprintf(stderr, "Failed to parse command\n");
        return -1;
    }

    if (p.we_wordc == 0) {
        wordfree(&p);
        return -1;
    }

    pid_t pid;
    int status;

    // 用 posix_spawn 执行
    if (posix_spawn(&pid, p.we_wordv[0], NULL, NULL, p.we_wordv, environ) != 0) {
        perror("posix_spawn");
        wordfree(&p);
        return -1;
    }

    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        wordfree(&p);
        return -1;
    }

    wordfree(&p);
    return status;
}

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

//获取文件大小
long get_file_size(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return st.st_size;
    } else {
        return -1;
    }
}
//逐字匹配文件是否相同,相同为1，不同为0，打开失败-1
int file_cmp(const char* path1, const char* path2){
    // long size1 = get_file_size(path1);
    // long size2 = get_file_size(path2);
    // if (size1 != size2) {
    //     return 0;
    // }
    FILE *f1 = fopen(path1, "rb");
    FILE *f2 = fopen(path2, "rb");
    if (!f1 || !f2) {
        if (f1) fclose(f1);
        if (f2) fclose(f2);
        return -1;
    }

    int result = 1;
    char buf1[4096], buf2[4096];
    size_t n1, n2;

    do {
        n1 = fread(buf1, 1, sizeof(buf1), f1);
        n2 = fread(buf2, 1, sizeof(buf2), f2);
        if (n1 != n2 || memcmp(buf1, buf2, n1) != 0) {
            result = 0;
            break;
        }
    } while (n1 > 0 && n2 > 0);

    fclose(f1);
    fclose(f2);
    return result;
}

//编译运行c++程序，成功数据库设为1，编译失败-2，结果错误-1，运行错误-3
void* cpp_runner(void* arg){
    Runner_task* runner_val = (Runner_task*)(arg);
    std::string path = runner_val->path;
    std::string file_name = runner_val->file_name;
    std::string porblem_title = runner_val->porblem_id;
    int test_num = runner_val->test_num;
    auto test_vec = runner_val->test_vec;
    std::string workpath = jsonManager::getManager().JsonGet()["isolate"]["workpath"];
    std::string cmd1 = std::string("/usr/bin/g++ ") + path + "/" + file_name + ".cpp" + " -o " + workpath + "/isolate_1/" + file_name + "__run";
    std::cout << cmd1 << std::endl;
    int status = system_spawn_cmd(cmd1.c_str());
    if(status == 0){
        int status = 0;

        fprintf(stderr, "编译成功\n");
        //复制到容器
        // std::cout << temp << std::endl;
        //容器运行
        for(int i = 0; i < test_num; i++){
            pthread_mutex_lock(&lock);
            fprintf(stderr, "单例启动\n");
            std::string cp_cmd = std::string("/usr/bin/cp ") + workpath + "/isolate_1/" + file_name + "__run" + " /var/local/lib/isolate/1/box/";
            status += system_spawn_cmd("/usr/local/bin/isolate --init --box-id=1");
            status += system_spawn_cmd(cp_cmd.c_str());
            std::string isolate_cmd_begin = std::string("/usr/local/bin/isolate --box-id=") + "1" + "   ";
            std::string isolate_cmd_processes = "--processes=1   ";
            std::string isolate_cmd_mem = "--mem=65536   ";
            std::string isolate_cmd_time = "--time=2   ";
            std::string isolate_cmd_stdin = "--stdin=input.txt   ";
            std::string isolate_cmd_stdout = "--stdout=output.txt   ";
            std::string isolate_cmd_stderr = "--stderr=error.txt   ";
            std::string isolate_cmd_last = "--run /box/" + file_name + "__run";
            std::string isolate_cmd;
            if(test_vec[i].first == "NULL"){
                isolate_cmd= isolate_cmd_begin + isolate_cmd_processes + isolate_cmd_mem 
                                    + isolate_cmd_time + isolate_cmd_stdout
                                    + isolate_cmd_stderr + isolate_cmd_last;
            }
            else{
                isolate_cmd= isolate_cmd_begin + isolate_cmd_processes + isolate_cmd_mem 
                                    + isolate_cmd_time + isolate_cmd_stdin + isolate_cmd_stdout
                                    + isolate_cmd_stderr + isolate_cmd_last;
            }


            status += system_spawn_cmd(isolate_cmd.c_str());
            //从容器复制输出
            cp_cmd = std::string("/usr/bin/cp /var/local/lib/isolate/1/box/output.txt ") + path + "/" + file_name + "_output_" + std::to_string(i) + ".txt";
            // std::cout << temp << std::endl;
            status += system_spawn_cmd(cp_cmd.c_str());
            //容器清理
            status += system_spawn_cmd("/usr/local/bin/isolate --cleanup --box-id=1");
            fprintf(stderr, "单例结束\n");
            status += pthread_mutex_unlock(&lock);
        }

        if(status == 0){
            //所有命令运行正常
            //比对输出是否正确
            for(int i = 0; i < test_num; i++){
                std::string stand_ansPath = std::string(jsonManager::getManager().JsonGet()["isolate"]["ansPath"]) + "/" + test_vec[i].second;
                std::string user_ansPath = path + "/" + file_name + "_output_" + std::to_string(i) + ".txt";
                std::cout << stand_ansPath << std::endl;
                std::cout << user_ansPath << std::endl;
                int cmp_code = file_cmp(stand_ansPath.c_str(), user_ansPath.c_str());
                if(cmp_code == 1){
                    //设置单测试用例状态
                    redis_set_field("Judge:" + file_name + "_" + std::to_string(i), "1");
                    
                    //设置多测试用例状态
                    redis_incr("Judge:" + file_name + "__sum");
                }
                else{
                    redis_set_field("Judge:" + file_name + "_" + std::to_string(i) , "-1");
                }
                redis_expire("Judge:" + file_name + "_" + std::to_string(i), 600);
            }
        }
        else{
                redis_set_field("Judge:" + file_name + "__sum" , "-3");
                redis_expire("Judge:" + file_name + "__sum", 600);
        }
        //删除多余资源
        status += system_spawn_cmd(("/usr/bin/rm " + path + "/" + file_name + ".cpp").c_str());
        status += system_spawn_cmd(("/usr/bin/rm " +  workpath + "/isolate_1/" + file_name + "__run").c_str());

        delete runner_val;
        // redis_expire("Judge:" + file_name, 600);
        return (void*)-1;
    }
    else{
        fprintf(stderr, "编译失败\n");
        redis_set_field("Judge:" + file_name, "-2");
        redis_expire("Judge:" + file_name, 600);
        return NULL;
    }
}


//Judge类
//上传用户答案
class runCpp_context{
public:
    std::string user_name;          //上传用户名
    std::string file_name;          //用户源文件名
    std::string porblem_id;         //题目id
    wfrest::HttpResp *resp;

};
//获取测试用例
void TaskPush(WFMySQLTask* mysqltask){
    SeriesWork* series = series_of(mysqltask);
    auto mysql_kv = mysqlRespCheck(mysqltask);
    runCpp_context* context = (runCpp_context*)series->get_context();
    int test_num = mysql_kv.size();
    if(test_num == 0){
        context->resp->set_status_code("409");
        return;
    }
    //添加任务
    std::vector<std::pair<std::string, std::string>> test_vec;
    for(auto& cow : mysql_kv){
        std::string input_path;
        std::string output_path;
        input_path = cow[0];
        output_path = cow[1];
        test_vec.push_back({input_path, output_path});

        fprintf(stderr, "测试用例%s排队中...\n",output_path.c_str());
    }
    //运行任务
    Runner_task* runner_val = new Runner_task;
    runner_val->path = std::string(jsonManager::getManager().JsonGet()["isolate"]["waiting_judge"]);
    runner_val->file_name = context->file_name;
    runner_val->porblem_id = context->porblem_id;
    runner_val->test_num = test_num;
    runner_val->test_vec = std::move(test_vec);
    isolate_pool->push(cpp_runner, (void*)runner_val);
    context->resp->set_status_code("200");
    return ;
}
//用户内容评判
void Judge::runCpp(const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series){
    std::string uri = url_decode((req->get_request_uri()));
    std::map<std::string, std::string> KVpairs = uriKV_get(uri);
    std::string porblem_title = KVpairs["porblem_title"];
    std::string code_type = KVpairs["code_type"];
    //验证cookie
    auto header = headerValueGet(req);
    std::string cookie = header["Cookie"];
    size_t start = cookie.find("=");
    size_t end = cookie.find(";");
    std::string token = std::string(jsonManager::getManager().JsonGet()["redis"]["prefix"]) + cookie.substr(0, end).substr(start + 1);
    std::string user_name = my_sync_check_token(token);
    if(user_name == ""){
        fprintf(stderr, "2\n");
        resp->set_status_code("403");
        return ;
    }
    else{

        //判断题目存在性
        std::string sql = "SELECT problem_id FROM  problems_cpp WHERE title = \"" + porblem_title + "\";";
        WFMySQLTask* mysqlTask = WFTaskFactory::create_mysql_task(jsonManager::getManager().JsonGet()["Mysql"]["sqlConnect"], 0, [req, resp, series, porblem_title, user_name](WFMySQLTask* mysqltask){
            auto sql_kv = mysqlRespCheck(mysqltask);
            if(sql_kv.size() > 0){
                //题目存在 接收题目
                std::string problems_path = jsonManager::getManager().JsonGet()["isolate"]["workpath"];
                std::string FilePath = problems_path + std::string("/waiting_Judge/") + user_name + "__" +porblem_title + ".cpp";
                std::cout << FilePath << std::endl;
                int fd = open(FilePath.c_str(), O_RDWR | O_CREAT, 0775);
                size_t size;
                const void* body;
                req->get_parsed_body(&body, &size);
                write(fd, body, size);
                close(fd);
                //编译运行代码
                // int state = cpp_runner("/home/shared/OJ_sys/OJ_Judge/isolate/waiting_Judge",  user_name + "__" +porblem_title);
                //设置redis，
                WFRedisTask* redisTask1 = WFTaskFactory::create_redis_task(jsonManager::getManager().JsonGet()["redis"]["redisIP"], 1, nullptr);
                // std::string temp = std::string(jsonManager::getManager().JsonGet()["redis"]["prefix"]) + token;
                redisTask1->get_req()->set_request("SET", {"Judge:" + user_name +"__" + porblem_title, "0"});
                series->push_back(redisTask1);
                WFRedisTask* redisTask2 = WFTaskFactory::create_redis_task(jsonManager::getManager().JsonGet()["redis"]["redisIP"], 1, nullptr);
                redisTask2->get_req()->set_request("EXPIRE", {"Judge:" + user_name +"__" + porblem_title, "1000"});
                series->push_back(redisTask2);
                WFRedisTask* redisTask4 = WFTaskFactory::create_redis_task(jsonManager::getManager().JsonGet()["redis"]["redisIP"], 1, nullptr);
                redisTask4->get_req()->set_request("SET", {"Judge:" + user_name +"__" + porblem_title + "__sum" , "0"});
                series->push_back(redisTask4);
                WFRedisTask* redisTask3 = WFTaskFactory::create_redis_task(jsonManager::getManager().JsonGet()["redis"]["redisIP"], 1, nullptr);
                redisTask3->get_req()->set_request("EXPIRE", {"Judge:" + user_name +"__" + porblem_title + "__sum" , "1000"});
                series->push_back(redisTask3);
                //找出测试用例
                std::string sql = "SELECT input_file_path, output_file_path FROM test_cases WHERE problem_id = \"" + porblem_title + "\";";
                std::cout << sql << std::endl;
                WFMySQLTask* mysqlTask = WFTaskFactory::create_mysql_task(jsonManager::getManager().JsonGet()["Mysql"]["sqlConnect"], 0, TaskPush);
                mysqlTask->get_req()->set_query(sql);
                series->push_back(mysqlTask);
                //添加共享结构体
                runCpp_context* context = new runCpp_context;
                context->user_name = user_name;
                context->file_name = user_name + "__" +porblem_title;
                context->porblem_id = porblem_title;
                context->resp = resp;
                series->set_context((void*)context);
                //填入待运行队列
                // Runner_task* runner_val = new Runner_task;
                // runner_val->path = "/home/shared/OJ_sys/OJ_Judge/isolate/waiting_Judge";
                // runner_val->file_name = ;
                // runner_val->porblem_title = porblem_title;
                // isolate_pool->push(cpp_runner, (void*)runner_val);
                resp->set_status_code("200");
                return ;
            }
            else{
                fprintf(stderr, "2\n");

                resp->set_status_code("409");
                return ;
            }
        });
        mysqlTask->get_req()->set_query(sql);
        series->push_back(mysqlTask);
        return ;
    }
}

//获取运行状态
void Judge::runState(const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series){
    std::string uri = url_decode((req->get_request_uri()));
    std::map<std::string, std::string> KVpairs = uriKV_get(uri);
    std::string porblem_title = KVpairs["porblem_title"];
    std::string code_type = KVpairs["code_type"];
    //验证cookie
    auto header = headerValueGet(req);
    std::string cookie = header["Cookie"];
    size_t start = cookie.find("=");
    size_t end = cookie.find(";");
    std::string token = std::string(jsonManager::getManager().JsonGet()["redis"]["prefix"]) + cookie.substr(0, end).substr(start + 1);
    std::string user_name = my_sync_check_token(token);
    if(user_name == ""){
        fprintf(stderr, "2\n");
        resp->set_status_code("403");
        return ;
    }
    std::string sql = "SELECT test_num FROM problems_cpp WHERE title = \""+ porblem_title +"\";";
    std::cout << sql << std::endl;
    WFMySQLTask* mysqlTask = WFTaskFactory::create_mysql_task(jsonManager::getManager().JsonGet()["Mysql"]["sqlConnect"], 0, [req, resp, series, user_name, porblem_title](WFMySQLTask* mysqltask){
        auto sql_kv = mysqlRespCheck(mysqltask);
        if(sql_kv.size() == 0){
            std::cout << "数据库返回为空" << std::endl;
            resp->set_status_code("409");
            return ;
        }
        //总述信息
        std::string test_num = sql_kv[0][0];
        std::string pass_num = my_sync_check_token(("Judge:" + user_name +"__" + porblem_title + "__sum").c_str());
        nlohmann::json j;
        j["total_test_num"] = test_num;
        j["total_pass_num"] = pass_num;
        for(int i = 0; i < std::stoi(test_num); i++){
            std::string redis_status_code = my_sync_check_token(("Judge:" + user_name +"__" + porblem_title + "_" + std::to_string(i)).c_str());
            // std::cout << "Judge:" + user_name +"__" + porblem_title + "_" + std::to_string(i) << std::endl;
            j[("problem_" + std::to_string(i)).c_str()] = redis_status_code;
        }
        std::string sql;
        if(test_num == pass_num){
            //用户此题成功完成
            sql = "INSERT INTO submissions (user_id, problem_id, language, result) VALUES(\""
                                + user_name + "\", \""
                                + porblem_title + "\", \"cpp\", \"Pass\");";

        }
        else{
            sql = "INSERT INTO submissions (user_id, problem_id, language, result) VALUES(\""
                                + user_name + "\", \""
                                + porblem_title + "\", \"cpp\", \"Unpass\");";
        }
        std::cout << sql << std::endl;
        WFMySQLTask* mysqlTask = WFTaskFactory::create_mysql_task(jsonManager::getManager().JsonGet()["Mysql"]["sqlConnect"], 0, NULL);
        mysqlTask->get_req()->set_query(sql);
        series->push_back(mysqlTask);

        resp->append_output_body(j.dump());
        resp->set_status_code("200");
    });
    mysqlTask->get_req()->set_query(sql);
    series->push_back(mysqlTask);
    return ;
    // //查看题目状态返回对应信息
    // //查看总信息
    // //查看单例信息
    // std::string redis_status_code = my_sync_check_token(("Judge:" + user_name +"__" + porblem_title).c_str());
    // std::cout << "Judge:" + user_name +"__" + "porblem_title" << std::endl;
    // std::cout << redis_status_code << std::endl;

    // if(redis_status_code == "1"){
    //     resp->append_output_body("pass");
    // }
    // else if(redis_status_code == "0"){
    //     resp->append_output_body("Unjudged");
    // }
    // else if(redis_status_code == "-1"){
    //     resp->append_output_body("Unpass");
    // }
    // else if(redis_status_code == "-2"){
    //     resp->append_output_body("Compilation_failed");
    // }
    // return ;
}

//添加题目，auther_id作废
void Judge::add_problem(const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series){
    std::string uri = url_decode((req->get_request_uri()));
    std::map<std::string, std::string> KVpairs = uriKV_get(uri);
    std::string problem_id = KVpairs["problem_id"];
    std::string problem_kind = KVpairs["problem_kind"];
    std::string problem_diffculty = KVpairs["problem_diffculty"];
    std::string time_limit = KVpairs["time_limit"];
    std::string mem_limit = KVpairs["mem_limit"];
    std::string authre_id = KVpairs["authre_id"];
    std::string is_public = KVpairs["is_public"];

    auto header = headerValueGet(req);
    std::string cookie = header["Cookie"];
    size_t start = cookie.find("=");
    size_t end = cookie.find(";");
    std::string token = std::string(jsonManager::getManager().JsonGet()["redis"]["prefix"]) + cookie.substr(0, end).substr(start + 1);
    // std::cout << "token:=====" << token  << "----yui" << std::endl;
    std::string Creater_user_id = my_sync_check_token(token);
    if(Creater_user_id == ""){
        resp->set_status_code("403");
        return;
    }
    else{
        std::string sql = "SELECT role FROM user_table WHERE username = \"" + Creater_user_id + "\";";
        WFMySQLTask* mysql_task = WFTaskFactory::create_mysql_task(jsonManager::getManager().JsonGet()["Mysql"]["sqlConnect"], 0, [req,series, resp,problem_id,problem_diffculty, time_limit, mem_limit, Creater_user_id, is_public](WFMySQLTask *mysqlTask){
            auto sql_kv = mysqlRespCheck(mysqlTask);
            if(sql_kv[0][0] == "student"){
                resp->set_status_code("403");
                return;
            }
            std::string sql = "SELECT problem_id FROM problems_cpp WHERE title = \"" + problem_id + "\";";
            WFMySQLTask* mysql_task = WFTaskFactory::create_mysql_task(jsonManager::getManager().JsonGet()["Mysql"]["sqlConnect"], 0, [req,series, resp,problem_id,problem_diffculty, time_limit, mem_limit, Creater_user_id, is_public](WFMySQLTask *mysqlTask){
                auto sql_kv =  mysqlRespCheck(mysqlTask);
                if(sql_kv.size() > 0){
                    resp->set_status_code("409");
                    return ;
                }
                //插入题目数据库
                std::string sql = "INSERT INTO problems_cpp (title, difficulty, time_limit, memory_limit, author_id, is_public) VALUES (\""
                                + problem_id + "\", \""
                                + problem_diffculty + "\", \""
                                + time_limit + "\", \""
                                + mem_limit + "\", \""
                                + Creater_user_id +"\", \""
                                + is_public + "\");";
                std::cout << sql << std::endl;
                WFMySQLTask* mysql_task = WFTaskFactory::create_mysql_task(jsonManager::getManager().JsonGet()["Mysql"]["sqlConnect"], 0, NULL);
                mysql_task->get_req()->set_query(sql);
                series->push_back(mysql_task);
                //传输题目
                std::string problems_path = jsonManager::getManager().JsonGet()["path"]["problems_path"];
                std::string FilePath = problems_path + std::string("/") + problem_id;
                int fd = open(FilePath.c_str(), O_RDWR | O_CREAT, 0666);
                size_t size;
                const void* body;
                req->get_parsed_body(&body, &size);
                write(fd, body, size);
                close(fd);
                resp->set_status_code("200");
                return;
            });
            std::cout << sql << std::endl;
            mysql_task->get_req()->set_query(sql);
            series->push_back(mysql_task);
        });
        std::cout << sql << std::endl;
        mysql_task->get_req()->set_query(sql);
        series->push_back(mysql_task);

        return;
    }

}
//上传题目测试用例
//样例输入
void Judge::add_test_in(const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series){
    std::string uri = url_decode((req->get_request_uri()));
    std::map<std::string, std::string> KVpairs = uriKV_get(uri);
    std::string problem_id = KVpairs["problem_id"];
    std::string case_id = KVpairs["problem_id"];

    auto header = headerValueGet(req);
    std::string cookie = header["Cookie"];
    size_t start = cookie.find("=");
    size_t end = cookie.find(";");
    std::string token = std::string(jsonManager::getManager().JsonGet()["redis"]["prefix"]) + cookie.substr(0, end).substr(start + 1);
    std::string Creater_user_id = my_sync_check_token(token);
    if(Creater_user_id == ""){
        resp->set_status_code("403");
        return;
    }
    else{
        std::string sql = "SELECT role FROM user_table WHERE username = \"" + Creater_user_id + "\";";
        WFMySQLTask* mysql_task = WFTaskFactory::create_mysql_task(jsonManager::getManager().JsonGet()["Mysql"]["sqlConnect"], 0, [req,series, resp,problem_id, case_id](WFMySQLTask *mysqlTask){
            auto sql_kv = mysqlRespCheck(mysqlTask);
            if(sql_kv[0][0] == "student"){
                resp->set_status_code("403");
                return;
            }
            //获取上传文件
            std::string dir_path = jsonManager::getManager().JsonGet()["isolate"]["inPath"];
            std::string in_case_name = problem_id + "input_" + case_id + ".txt";
            int fd = open((dir_path + "/" + in_case_name).c_str(), O_RDWR | O_CREAT, 0666);
            size_t size;
            const void* body;
            req->get_parsed_body(&body, &size);
            write(fd, body, size);
            close(fd);
            //设置redis值
            redis_set_field(in_case_name.c_str(), 1);

        });
        std::cout << sql << std::endl;
        mysql_task->get_req()->set_query(sql);
        series->push_back(mysql_task);
        return;
    }
}
//样例输出
void Judge::add_test_out(const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series){
}

//获取题目列表
void Judge::get_problem_list(const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series){
    std::string uri = url_decode((req->get_request_uri()));
    auto form_kv = uriKV_get(uri);
    std::string beginID = form_kv["beginID"];
    auto header = headerValueGet(req);
    std::string cookie = header["Cookie"];
    size_t start = cookie.find("=");
    size_t end = cookie.find(";");
    std::string token = std::string(jsonManager::getManager().JsonGet()["redis"]["prefix"]) + cookie.substr(0, end).substr(start + 1);
    std::string user_name = my_sync_check_token(token);
    if(user_name == ""){
        resp->set_status_code("403");
    }
    else{
        std::string sql = "SELECT title, difficulty, author_id  FROM problems_cpp WHERE problem_id >= "
                            + beginID + " AND is_public = 1 LIMIT 10;";
        std::cout << sql << std::endl;
        WFMySQLTask* mysqlTask = WFTaskFactory::create_mysql_task(jsonManager::getManager().JsonGet()["Mysql"]["sqlConnect"], 0, [resp](WFMySQLTask* mysqltask){
            auto sql_kv = mysqlRespCheck(mysqltask);
            fprintf(stderr, "sql_kvsize: %d", sql_kv.size());
            nlohmann::json j_array = nlohmann::json::array();
            for(const auto& row : sql_kv){
                nlohmann::json j_row = nlohmann::json::array();
                for(const auto& col : row){
                    j_row.push_back(col);
                }
                j_array.push_back(j_row);
            }
            resp->append_output_body(j_array.dump());
            resp->set_status_code("200");
        });
        mysqlTask->get_req()->set_query(sql);
        series->push_back(mysqlTask);
    }

}
