#include "linuxHeader.h"

//任务共享数据函数
struct SeriesContext{
    std::string username;
    std::string password;
    std::string role;
    std::string token;
    wfrest::HttpResp *resp;
};

class UserInfo{
public:
    static void getUserName (const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series);
    //账号注册
    static void userRegist (const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series);
    //账号登录
    static void userLogin (const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series);
};