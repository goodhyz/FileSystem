/**
 * share_memory.h
 * 共享内存结构体以及一些常量
 */

#pragma once

#include <cstring>
#include <iostream>
#include <windows.h>
#include <sstream>
#include <conio.h> 
#include <cstdint>
#include <algorithm>
struct User {
    std::string username;
    uint32_t uid;
    uint32_t gid;

    User(){}
    User(std::string _username, uint32_t _uid, uint32_t _gid) {
        username = _username;
        uid = _uid;
        gid = _gid;
    }
    void set(std::string _username,uint32_t _uid, uint32_t _gid) {
        username = _username;
        uid = _uid;
        gid = _gid;
    }
};

struct SharedMemory {
    char command[256];// 命令
    char result[4096];// 服务端返回的结果

    int cur_user; // 当前用户
    int cur_dir_inode_id; // 当前目录的inode id
    User user_list[10];//已登录的用户

    /*用于命令通信*/
    bool ready; // 客户端发出请求，同时作为命令锁变量
    bool done;

    /*用于登录*/
    bool is_login_prompt; //是否发送登录请求，同时作为登录锁变量
    bool is_login_success;
    bool is_login_fail;

    int get_free_user() {
        for (int i = 0; i < 10; ++i) {
            if (user_list[i].username.empty()) {
                return i;
            }
        }
        return -1;
    }
};