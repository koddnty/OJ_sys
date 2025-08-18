#include "judge.h"
#include <pthread.h>


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

int cpp_runner(std::string& path){
    std::string workpath = jsonManager::getManager().JsonGet()["isolate"]["workpath"];
    std::string cmd1 = "/usr/bin/g++ " + path + " -o " + workpath + "isolate1/";
    std::cout << cmd1 << std::endl;
    int status = system_spawn_cmd(cmd1.c_str());
    if(status == 0){
        pthread_mutex_lock(&lock);
        fprintf(stderr, "编译成功\n");
        system_spawn_cmd("/usr/local/bin/isolate --init --box-id=0");
        system_spawn_cmd("/usr/bin/cp /home/shared/OJ_sys/OJ_Judge/test_isolate/exec/test /var/local/lib/isolate/0/box/");
        system_spawn_cmd("/usr/local/bin/isolate --box-id=0 \
            --processes=1 \
            --mem=65536 \
            --time=2 \
            --stdout=output.txt \
            --stderr=error.txt \
            --run /box/test");
        system_spawn_cmd("/usr/bin/cp /var/local/lib/isolate/0/box/output.txt ./output.txt");
        system_spawn_cmd("/usr/local/bin/isolate --cleanup --box-id=1");
        pthread_mutex_unlock(&lock);
        return 0;
    }
    else{
        return 1;
    }
}

//Judge类
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
        std::cout << sql << std::endl;
        WFMySQLTask* mysqlTask = WFTaskFactory::create_mysql_task(jsonManager::getManager().JsonGet()["Mysql"]["sqlConnect"], 0, [req, resp, porblem_title, user_name](WFMySQLTask* mysqltask){
            auto sql_kv = mysqlRespCheck(mysqltask);
            if(sql_kv.size() > 0){
                //题目存在 接收题目
                std::string problems_path = jsonManager::getManager().JsonGet()["isolate"]["workpath"];
                std::string FilePath = problems_path + std::string("/waiting_Judge/") + user_name + "_:_" +porblem_title;
                std::cout << FilePath << std::endl;
                int fd = open(FilePath.c_str(), O_RDWR | O_CREAT, 0666);
                size_t size;
                const void* body;
                req->get_parsed_body(&body, &size);
                write(fd, body, size);
                close(fd);
                //编译运行代码
                cpp_runner();
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
//添加题目，auther_id作废
void Judge::add(const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series){
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
        //添加题目
        std::string sql = "SELECT problem_id FROM problems_cpp WHERE title = \"" + problem_id + "\";";
        WFMySQLTask* mysql_task = WFTaskFactory::create_mysql_task(jsonManager::getManager().JsonGet()["Mysql"]["sqlConnect"], 0, [req,series, resp,problem_id,problem_diffculty, time_limit, mem_limit, Creater_user_id, is_public](WFMySQLTask *mysqlTask){
            auto sql_kv =  mysqlRespCheck(mysqlTask);
            if(sql_kv.size() > 0){
                resp->set_status_code("409");
                return ;
            }
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

        return;
    }

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
