#include <windows.h>
#include <iostream>
#include <cstring>
#include "share_memory.h"

int main() {
    // 打开内存映射文件
    HANDLE hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, "SimdiskSharedMemory");
    if (hMapFile == NULL) {
        std::cerr << "Could not open file mapping object: " << GetLastError() << std::endl;
        return 1;
    }

    // 映射到进程的地址空间
    SharedMemory* shm = (SharedMemory*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemory));
    if (shm == NULL) {
        std::cerr << "Could not map view of file: " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        return 1;
    }
    std::cout<<welcome1;
    std::cout << "Welcome to Simdisk Shell!" << std::endl;
    std::cout << "Type 'exit' to quit." << std::endl;

    while (true) {
        std::cout << shm->result;
        std::string command;
        std::getline(std::cin, command);

        // 写入命令并通知后台
        strncpy(shm->command, command.c_str(), sizeof(shm->command) - 1);
        shm->ready = true;

        // 等待结果
        while (!shm->done) {
            Sleep(10); // 避免忙等待
        }

        // 读取结果并打印
        std::cout << "Result: " << shm->result << std::endl;

        // 重置状态
        shm->done = false;

        if (command == "exit") break;
    }

    // 清理资源
    UnmapViewOfFile(shm);
    CloseHandle(hMapFile);

    return 0;
}