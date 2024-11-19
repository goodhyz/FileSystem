#include <iostream>
#include <windows.h>
#include <string>
struct SharedMemory {
    char command[256];
    char result[256];
    HANDLE command_sem;
    HANDLE result_sem;
};

int main() {
    const char *shm_name = "simdisk_shm";
    HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, shm_name);
    if (hMapFile == NULL) {
        std::cerr << "Failed to open shared memory.\n";
        return 1;
    }

    SharedMemory *shm = (SharedMemory *)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemory));
    if (shm == NULL) {
        std::cerr << "Failed to map shared memory.\n";
        CloseHandle(hMapFile);
        return 1;
    }

    while (true) {
        std::cout<<welcome;
        std::cout << "simdisk> ";
        std::string command;
        std::getline(std::cin, command);

        strncpy(shm->command, command.c_str(), sizeof(shm->command) - 1);

        // 通知Simdisk有新命令
        ReleaseSemaphore(shm->command_sem, 1, NULL);

        // 等待Simdisk处理结果
        WaitForSingleObject(shm->result_sem, INFINITE);
        std::cout << "Result: " << shm->result << std::endl;

        if (command == "exit") break;
    }

    UnmapViewOfFile(shm);
    CloseHandle(hMapFile);

    return 0;
}