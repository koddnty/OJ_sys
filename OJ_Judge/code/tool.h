#pragma once
#include "linuxHeader.h"





//项目json信息
// jsonManager jsonManager::s_;
class jsonManager{
public:
    static jsonManager& getManager(){
        return s_;
    } 
    void print(){
        std::cout << "json\n" << (this->j_obj).dump(4) << std::endl;
    }
    jsonManager(jsonManager&) = delete;
    jsonManager(jsonManager&&) = delete;

    void jsonLoad(std::string& path){
        std::fstream in(path);
        if(!in.is_open()){
            std::cout << "文件打开失败" << std::endl;
            return; 
        }
        try {
            in >> j_obj;
        } catch (const std::exception& e) {
            std::cerr << "解析 JSON 出错: " << e.what() << std::endl;
        }
    }
    const nlohmann::json& JsonGet(){
        return j_obj;
    }

private:
    static jsonManager s_;
    jsonManager(){}
    nlohmann::json j_obj;
};


//uri解析函数
std::map<std::string, std::string> uriKV_get(std::string uri);
//浏览器字符编码解析
std::string url_decode(const std::string str);
//mysql数据库信息粗提取
std::vector<std::vector<std::string>> mysqlRespCheck(WFMySQLTask* mysqlTask );
//获取请求头
std::map<std::string, std::string> headerValueGet(const wfrest::HttpReq *req);
//验证token输入token（不包"含token="部分）返回用户id
std::string  my_sync_check_token(const std::string& token);