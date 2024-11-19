#include "share_memory.h"
#include <cstring>
#include <iostream>
#include <windows.h>

int main() {
    // 打开内存映射文件
    HANDLE hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, "SimdiskSharedMemory");
    if (hMapFile == NULL) {
        std::cerr << __ERROR << "后端服务失联, 请检查正常后重启" << __NORMAL << std::endl;
        // return 1;
    }

    // 映射到进程的地址空间
    SharedMemory *shm = (SharedMemory *)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemory));
    if (shm == NULL) {
        std::cerr << __ERROR << "创建内存映射失败" << __NORMAL << std::endl;
        CloseHandle(hMapFile);
        // return 1;
    }

    // 程序的进入页面
    std::cout << welcome1<<std::endl;
    std::cout << "键入 'help' 获得帮助" << std::endl;
    std::cout << "键入 'exit' 停止程序" << std::endl;
    std::cout << shm->result;
    while (true) {
        std::string command;
        std::getline(std::cin, command);
        // 确保后端程序在线
        if (hMapFile == NULL) {
            std::cerr << __ERROR << "后端服务失联, 请检查正常后重启" << __NORMAL << std::endl;
            return 1;
        }
        if (command == "help") {
            std::cout << "命令列表:" << std::endl;
            std::cout << "    help: 显示帮助信息" << std::endl;
            std::cout << "    exit: 退出程序" << std::endl;
            std::cout << "    clear: 清空屏幕" << std::endl;
            std::cout << "    其他: 执行命令" << std::endl;
            continue;
        }
        // 写入命令并通知后台
        strncpy(shm->command, command.c_str(), sizeof(shm->command) - 1);
        shm->ready = true;

        // 等待结果
        while (!shm->done) {
            Sleep(10); // 避免忙等待
        }
        if (command == "exit")
            break;
        else if (command == "clear")
            system("cls");
        // 读取结果并打印
        std::cout << shm->result;

        // 重置状态
        shm->done = false;
    }

    // 清理资源
    UnmapViewOfFile(shm);
    CloseHandle(hMapFile);

    return 0;
}