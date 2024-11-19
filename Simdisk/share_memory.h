#pragma once

#include <cstring>
#include <iostream>
#include <windows.h>

/**
 * 共享内存结构体, 用于进程间通信
 * command: 命令
 * result: 服务端返回的结果
 * ready: 客户端是否给出命令
 * done: 服务端是否处理完命令
 */
struct SharedMemory {
    char command[256];
    char result[4096];
    bool ready;
    bool done;
};