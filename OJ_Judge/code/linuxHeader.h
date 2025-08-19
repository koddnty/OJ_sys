#pragma once

#include <map>
#include <vector>
#include <stdio.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <algorithm>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <nlohmann/json.hpp>
#include <wfrest/HttpServer.h>
#include <wfrest/HttpMsg.h>
#include <workflow/poller.h>
#include <workflow/HttpUtil.h>
#include <workflow/Workflow.h>
#include <workflow/StringUtil.h>
#include <workflow/MySQLResult.h>
#include <workflow/WFFacilities.h>
#include <workflow/WFHttpServer.h>
#include <sw/redis++/redis++.h>
#include <pthread.h>

#include "pthread_pool.h"

#define MAX_ISOLATE_MUTEX_LOCK 3

class ThreadPool;
extern ThreadPool* isolate_pool;

extern pthread_mutex_t* lock_list;

//运行任务
class Runner_task{
public:
    std::string path;
    std::string file_name;
    std::string porblem_id;
    int test_num;           //测试用例个数
    std::vector<std::pair<std::string, std::string>> test_vec;     //测试用例
};
