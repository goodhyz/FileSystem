// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <sys/ipc.h>
// #include <sys/shm.h>
// #include <sys/types.h>

// #define SHM_KEY 1234  // 共享内存的键值

// // 假设这部分代码是 simdisk 中与共享内存交互的部分

// // 初始化共享内存
// void *init_shared_memory() {
//     int shmid = shmget(SHM_KEY, sizeof(struct DiskInfo), IPC_CREAT | 0666);
//     if (shmid < 0) {
//         perror("shmget");
//         exit(1);
//     }
//     return shmat(shmid, NULL, 0);
// }

// // 处理shell的命令
// void process_command(char *command, void *shared_memory) {
//     // 解析命令并进行操作，例如创建文件、删除文件、读写文件等
//     if (strcmp(command, "status") == 0) {
//         // 假设获取磁盘状态并更新共享内存
//         update_disk_status(shared_memory);
//     } else if (strncmp(command, "create", 6) == 0) {
//         // 创建文件操作
//         create_file(command, shared_memory);
//     } else if (strncmp(command, "delete", 6) == 0) {
//         // 删除文件操作
//         delete_file(command, shared_memory);
//     } else {
//         printf("Unknown command: %s\n", command);
//     }
// }

// // 主函数
// int main() {
//     // 初始化共享内存
//     void *shared_memory = init_shared_memory();
    
//     // 模拟等待并监听shell命令
//     char command[256];
//     while (1) {
//         printf("simdisk> ");
//         fgets(command, sizeof(command), stdin);
//         command[strcspn(command, "\n")] = 0;  // 去除末尾换行
        
//         // 处理用户输入的命令
//         process_command(command, shared_memory);
//     }
    
//     return 0;
// }



