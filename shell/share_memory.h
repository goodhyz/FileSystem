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

const std::string welcome1 = 
"    ____  __  __        _____            __                        \n"
"   / __/ /_/ / /__     / ___/__  _______/ /____  ____ ___          \n"
"  / /_  / / / / _ \\    \\__ \\/ / / / ___/ __/ _ \\/ __ `__ \\    \n"
" / __/ / / / /  __/   ___/ / /_/ (__  / /_/  __/ / / / / /         \n"
"/_/   /_/ /_/\\___/   /____/\\__, /____/\\__/\\___/_/ /_/ /_/      \n"
"                          /____/                                 \n"
"                                           --Provided by Hu Yuzhi  \n";

std::string welcome2 =
"      ___                   ___  ___                   ___      ___         ___      ___         ___         ___               ___                     \n"
"     /\\  \\        ___      /\\__\\/\\  \\                 /\\  \\    |\\__\\       /\\  \\    /\\  \\       /\\  \\       /\\__\\                    \n"
"    /::\\  \\      /\\  \\    /:/  /::\\  \\               /::\\  \\   |:|  |     /::\\  \\   \\:\\  \\     /::\\  \\     /::|  |                      \n"
"   /:/\\:\\  \\     \\:\\  \\  /:/  /:/\\:\\  \\             /:/\\ \\  \\  |:|  |    /:/\\ \\  \\   \\:\\  \\   /:/\\:\\  \\   /:|:|  |                \n"
"  /::\\~\\:\\  \\    /::\\__\\/:/  /::\\~\\:\\  \\           _\\:\\~\\ \\  \\ |:|__|__ _\\:\\~\\ \\  \\  /::\\  \\ /::\\~\\:\\  \\ /:/|:|__|__         \n"
" /:/\\:\\ \\:\\__\\__/:/\\/__/:/__/:/\\:\\ \\:\\__\\         /\\ \\:\\ \\ \\__\\/::::\\__/\\ \\:\\ \\ \\__\\/:/\\:\\__/:/\\:\\ \\:\\__/:/ |::::\\__\\  \n"
" \\/__\\:\\ \\/__/\\/:/  /  \\:\\  \\:\\~\\:\\ \\/__/         \\:\\ \\:\\ \\/__/:/~~/~  \\:\\ \\:\\ \\/__/:/  \\/__\\:\\~\\:\\ \\/__\\/__/~~/:/  /     \n"
"      \\:\\__\\ \\::/__/    \\:\\  \\:\\ \\:\\__\\            \\:\\ \\:\\__\\/:/  /     \\:\\ \\:\\__\\/:/  /     \\:\\ \\:\\__\\       /:/  /         \n"
"       \\/__/  \\:\\__\\     \\:\\  \\:\\ \\/__/             \\:\\/:/  /\\/__/       \\:\\/:/  /\\/__/       \\:\\ \\/__/      /:/  /                  \n"
"               \\/__/      \\:\\__\\:\\__\\                \\::/  /              \\::/  /              \\:\\__\\       /:/  /                          \n"
"                           \\/__/\\/__/                 \\/__/                \\/__/                \\/__/       \\/__/                                \n"
"                                                                                                                               --Provided by Hu Yuzhi  \n";

// 便于阅读的颜色
const std::string __ERROR = "\033[31m";
const std::string __SUCCESS = "\033[36m";
const std::string __PATH = "\033[01;34m";
const std::string __NORMAL = "\033[0m";
const std::string __USER = "\033[01;32m";

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