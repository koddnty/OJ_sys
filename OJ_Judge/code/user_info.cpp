#include "user_info.h"
#include "tool.h"



void UserInfo::getUserName(const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series){

}

//账号注册
void UserInfo::userRegist (const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series){
    resp->add_header_pair("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    resp->add_header_pair("Access-Control-Allow-Headers", "Content-Type");
    resp->add_header_pair("Access-Control-Allow-Origin", "*");
    auto form_kv = req->form_kv();
    auto header = headerValueGet(req);
    SeriesContext* context = new SeriesContext;
    context->username = form_kv["username"];
    // context->password = form_kv["password"];
    context->role = form_kv["role"];
    std::string salt = "73256";
    char* encryptPassWord = crypt(form_kv["password"].c_str(), "$5$73256");
    fprintf(stderr, "%s\n", encryptPassWord);
    //处理cookie
    std::string cookie = header["Cookie"];
    size_t start = cookie.find("=");
    size_t end = cookie.find(";");
    std::string token = std::string(jsonManager::getManager().JsonGet()["redis"]["prefix"]) + cookie.substr(0, end).substr(start + 1);
    std::cout << "token:=====" << token  << "----yui" << std::endl;
    std::string Creater_user_id = my_sync_check_token(token);
    if(((Creater_user_id == "admin") || 
        (Creater_user_id == "teacher" && context->role == "student"))&&
        (context->role == "student" || context->role == "admin" || context->role == "teacher"))
    {
        //允许创建账户
        std::string sql = "INSERT INTO user_table (username, password_hash, role, email) VALUES (\""
        + context->username + "\", \""
        + encryptPassWord + "\", \""
        + context->role + "\", \"000\");";
        std::cout << sql << std::endl;
        WFMySQLTask* mysqlTask = WFTaskFactory::create_mysql_task(jsonManager::getManager().JsonGet()["Mysql"]["sqlConnect"], 0, NULL);
        mysqlTask->get_req()->set_query(sql);
        series->push_back(mysqlTask);
        resp->set_status_code("200");
    }
    else{
        resp->set_status_code("403");
    }
}

// 账号登录
std::string generateToken(std::string username){
    std::string salt = "945168";
    std::string data = username + salt;
    
    // 初始化EVP上下文
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_md5();
    
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len;
    
    // 执行MD5哈希计算
    EVP_DigestInit_ex(mdctx, md, nullptr);
    EVP_DigestUpdate(mdctx, data.c_str(), data.length());
    EVP_DigestFinal_ex(mdctx, digest, &digest_len);
    EVP_MD_CTX_free(mdctx);
    
    // 转换为十六进制字符串
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < digest_len; ++i) {
        oss << std::setw(2) << static_cast<int>(digest[i]);
    }
    time_t now_time = time(nullptr);
    struct tm *ptm = localtime(&now_time);
    char timeStamp[20] = {0};
    snprintf(timeStamp, 20, "%02d_0_%02d_0_%02d", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
    return oss.str() + std::string(timeStamp);
}       //由豆包完成，用md5计算返回给用户的token
void* login_mysqlCallBack(WFMySQLTask *mysqlTask){  
    SeriesWork* series = series_of(mysqlTask);
    SeriesContext* context = (SeriesContext*)(series->get_context());
    wfrest::HttpResp* HttpResp = context->resp;
    auto form = mysqlRespCheck(mysqlTask);
    if(form.empty() || form[0].empty()){
        HttpResp->String("Failed");
        context->resp->set_status_code("401");
        return NULL;
    }
    std::string role = form[0][0];
    std::string username = form[0][1];
    std::string token = generateToken(context->username);
    std::cout << username << std::endl;
    //token存放
    WFRedisTask* redisTask1 = WFTaskFactory::create_redis_task(jsonManager::getManager().JsonGet()["redis"]["redisIP"], 1, nullptr);
    std::string temp = std::string(jsonManager::getManager().JsonGet()["redis"]["prefix"]) + token;
    redisTask1->get_req()->set_request("SET", {temp, username});
    series->push_back(redisTask1);
    WFRedisTask* redisTask2 = WFTaskFactory::create_redis_task(jsonManager::getManager().JsonGet()["redis"]["redisIP"], 1, nullptr);
    redisTask2->get_req()->set_request("EXPIRE", {temp, "259200"});
    series->push_back(redisTask2);
    // std::cout << token << std::endl;
    //设置返回值
    HttpResp->String(role.c_str());
    HttpResp->add_header("Set-Cookie", "token=" + token + "; Path=/; HttpOnly; SameSite=Lax");
    return NULL;
}
void UserInfo::userLogin (const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series){
    resp->add_header_pair("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    resp->add_header_pair("Access-Control-Allow-Headers", "Content-Type");
    resp->add_header_pair("Access-Control-Allow-Origin", "*");
    auto form_kv = req->form_kv();
    // std::string username = url_decode(form_kv["username"]);
    // std::string password = url_decode(form_kv["password"]);
    std::cout << url_decode(form_kv["username"]) << std::endl;
    std::cout << url_decode(form_kv["password"]) << std::endl;
    SeriesContext* context = new SeriesContext;
    context->username = url_decode(form_kv["username"]);
    std::string password = url_decode(form_kv["password"]);
    std::string salt = "73256";
    context->password = crypt(form_kv["password"].c_str(), "$5$73256");
    fprintf(stderr, "%s\n", context->password);
    context->resp = resp;
    series->set_context((void*)context);
    //查询数据库
    std::string sql = "SELECT role, username FROM user_table WHERE username = \""
                         + context->username
                         + "\" AND password_hash = \"" 
                         + context->password + "\";";
    std::cout << sql << std::endl;
    WFMySQLTask* mysqlTask = WFTaskFactory::create_mysql_task(jsonManager::getManager().JsonGet()["Mysql"]["sqlConnect"], 0, login_mysqlCallBack);
    mysqlTask->get_req()->set_query(sql);
    series->push_back(mysqlTask);
}