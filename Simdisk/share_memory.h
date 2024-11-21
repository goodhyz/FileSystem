#pragma once

#include <cstring>
#include <iostream>
#include <windows.h>
#include <csignal>
#include <mutex>
#include "simdisk.h"


// 每个用户的信息
struct UserShareMemory{
    User user;
    int cur_dir_inode_id; // 当前目录的inode id
    int cur_user; // 当前用户
    char command[256];// 命令
    char result[4096];// 服务端返回的结果
    bool ready; // 客户端发出请求，同时作为命令锁变量
    bool done;

    /*用于登录*/
    bool is_login_prompt; //是否发送登录请求，同时作为登录锁变量
    bool is_login_success;
    bool is_login_fail;
};

struct OpenedFile{
    int inode_id; // 文件的inode id
    bool is_write; // 是否写
};

struct OpenedFileTable{
    OpenedFile opened_file[10]; // 最多10个打开文件
    int get_free_file() {
        for (int i = 0; i < 10; ++i) {
            if (opened_file[i].inode_id == -1) {
                return i;
            }
        }
        return -1;
    }
    void add_file(int inode_id, bool is_write) {
        int free_file = get_free_file();
        if (free_file != -1) {
            opened_file[free_file].inode_id = inode_id;
            opened_file[free_file].is_write = is_write;
        }
    }
    void close_file(int inode_id) {
        for (int i = 0; i < 10; ++i) {
            if (opened_file[i].inode_id == inode_id) {
                opened_file[i].inode_id = -1;
                opened_file[i].is_write = false;
                break;
            }
        }
    }
    bool is_writing(int inode_id) {
        for (int i = 0; i < 10; ++i) {
            if (opened_file[i].inode_id == inode_id) {
                return opened_file[i].is_write;
            }
        }
        return false;
    }
};

/**
 * 共享内存结构体, 用于进程间通信
 * command: 命令
 * result: 服务端返回的结果
 * ready: 客户端是否给出命令
 * done: 服务端是否处理完命令
 */
struct SharedMemory {
    UserShareMemory user_list[10]; // 最多10个用户
    OpenedFileTable open_file_table; // 最多10个打开文件
    int get_free_user() {
        for (int i = 0; i < 10; ++i) {
            if (user_list[i].user.username.empty()) {
                return i;
            }
        }
        return -1;
    }
};