#pragma once

#include <cstring>
#include <iostream>
#include <windows.h>
#include <csignal>
#include "simdisk.h"

/**
 * 共享内存结构体, 用于进程间通信
 * command: 命令
 * result: 服务端返回的结果
 * ready: 客户端是否给出命令
 * done: 服务端是否处理完命令
 */
struct SharedMemory {
    char command[256];// 命令
    char result[4096];// 服务端返回的结果

    int cur_user; // 当前用户
    int cur_dir_inode_id; // 当前目录的inode id
    User user_list[10];// 已登录的用户

    /*用于命令通信*/
    bool ready;
    bool done;

    /*用于登录*/
    bool is_login_prompt;
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