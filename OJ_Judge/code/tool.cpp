#include "tool.h"
#include "user_info.h"


//uri解析函数
std::map<std::string, std::string> uriKV_get(std::string uri){
    using namespace std;
   std::map<std::string, std::string> uriKV;
    //分开"?"
    string allKV = move(uri.substr(uri.find('?') + 1));
    int total = count(allKV.begin(), allKV.end(), '&') + 1;
    for(int i = 0; i < total; i++){
        int len = allKV.find('&');
        string singleKV = allKV.substr(0, len);
        allKV = allKV.substr(len + 1);

        string key = singleKV.substr(0, singleKV.find('='));
        string value = singleKV.substr(singleKV.find('=') + 1);

        uriKV.insert({key, value});
    }
    return uriKV;
}


//浏览器字符编码解析
std::string url_decode(const std::string str) {
    std::string decoded;
    char ch;
    int i, ii;
    for (i = 0; i < str.length(); ++i) {
        if (str[i] == '%') {
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            decoded += ch;
            i = i + 2;
        } else if (str[i] == '+') {
            decoded += ' ';
        } else {
            decoded += str[i];
        }
    }
    return decoded;
}
//mysql数据库信息粗提取
std::vector<std::vector<std::string>> mysqlRespCheck(WFMySQLTask* mysqlTask ){
    SeriesWork* series = series_of(mysqlTask);
    // UserListInfo* context = (UserListInfo*)series->get_context();
    std::vector<std::vector<std::string>> valueVec;
    //1.判断任务状态
    int state = mysqlTask->get_state();
    if(state != WFT_STATE_SUCCESS){
        fprintf(stderr, "task error = %d\n", mysqlTask->get_error());
        return valueVec;
    }
    //2.判断回复包类型
    protocol::MySQLResponse* resp = mysqlTask->get_resp();
    int packType = resp->get_packet_type();
    if(packType == MYSQL_PACKET_ERROR){
        fprintf(stderr, "packet error = %d \n\t %s \n", 
                        mysqlTask->get_resp()->get_error_code(),
                        mysqlTask->get_resp()->get_error_msg().c_str());
    }

    //3.遍历结果集
    protocol::MySQLResultCursor mysqlCursor (mysqlTask->get_resp());
    //3.1判断结果集状态
    do{        
        int status = mysqlCursor.get_cursor_status();
        if(status == MYSQL_STATUS_ERROR){fprintf(stderr, "mysqlRespCheck__结果集解析错误");}
        //获取每个field
        const protocol::MySQLField *const * fields = mysqlCursor.fetch_fields();
        std::vector<std::vector< protocol::MySQLCell>> rows;
        mysqlCursor.fetch_all(rows);
        for (unsigned int  i = 0; i < rows.size(); i++){
            // step-12. 具体每个cell的读取
            // fprintf(stderr, "%s   %s\n", rows[i][0].as_string().c_str(), rows[i][1].as_string().c_str());
            std::vector<std::string> temp;
            for(size_t j = 0; j < rows[i].size(); j++){
                temp.push_back(rows[i][j].as_string());
                // fprintf(stderr, "%s   ", rows[i][j].as_string().c_str());
            }
            // fprintf(stderr, "\n");
            valueVec.push_back(temp);
        }
    }while(mysqlCursor.next_result_set());
    return valueVec;
}
//获取请求头入cookie
std::map<std::string, std::string> headerValueGet(const wfrest::HttpReq *req){
    std::string name, value;
    std::map<std::string, std::string> header_map;
    protocol::HttpHeaderCursor ReqCursor(req);
    while (ReqCursor.next(name, value)){
        header_map.insert({name, value});
    }
    return header_map;
}
//验证token输入token（不包"含token="部分）返回用户id
std::string  my_sync_check_token(const std::string& token) {

    try {
        // 连接 Redis（可缓存这个对象，不要每次创建）
        std::string redisIP = jsonManager::getManager().JsonGet()["redis"]["redisIP"];
        sw::redis::Redis realRedis = sw::redis::Redis(redisIP);
        sw::redis::Redis* redis = &(realRedis);

        // token 一般保存在类似 token:xxx = username 的键值中
        std::optional<std::string> val = redis->get(token);
        // fprintf(stderr, "%s\n", token.c_str());
        if(val.has_value()){
            return val.value();
        }  // 有值说明 token 有效
        else{
            return "";
        }
    } catch (const sw::redis::Error &err) {
        // 连接失败或其他 Redis 错误
        std::cout << "Redis error: " <<  std::endl;
        std::cerr << "Redis error: " << err.what() << std::endl;
        // fprintf(stderr, "==========================n\n");
        return "";
    }
}