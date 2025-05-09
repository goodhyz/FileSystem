/**
 * @file shell.cpp
 * @brief 用于用户交互的shell
 * @author Hu Yuzhi
 * @date 2024-11-20
 */

// #include "shell.h"
// 已经在share_memory.h中包含了shell.h
#include "share_memory.h"

void print_help(SharedMemory *shm, int cur_user);

int main() {
    // 打开内存映射文件
    HANDLE hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, "SimdiskSharedMemory");
    if (hMapFile == NULL) {
        std::cerr << __ERROR << "后端服务失联, 请检查正常后重启" << __NORMAL << std::endl;
        return 1;
    }

    // 映射到进程的地址空间
    SharedMemory *shm = (SharedMemory *)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemory));
    if (shm == NULL) {
        std::cerr << __ERROR << "创建内存映射失败" << __NORMAL << std::endl;
        CloseHandle(hMapFile);
        return 1;
    }
    // 记录当前是哪个用户
    int cur_label, cur_dir_inode_id = 0;
    // 获取一个空闲用户, 访问用户列表
    cur_label = shm->get_free_user();
    if (cur_label == -1) {
        std::cerr << __ERROR << "用户数量已达上限" << __NORMAL << std::endl;
        return 1;
    }
    shm->user_list[cur_label].cur_user = cur_label;
    while (true) {
        shm->user_list[cur_label].is_login_fail = shm->user_list[cur_label].is_login_success = false;
        // 输入账号密码
        std::cout << "账号：";
        std::string user;
        std::cin >> user;
        // 去除空格
        user.erase(std::remove_if(user.begin(), user.end(), ::isspace), user.end());

        std::cout << "密码：";
        std::string password;
        // std::cin>>password;
        char ch;
        while ((ch = _getch()) != '\r') {          // 读取字符直到回车
            if (ch == '\b' && !password.empty()) { // 处理退格键
                std::cout << "\b \b";
                password.pop_back();
            } else if (ch != '\b') {
                std::cout << '*'; // 显示星号
                password.push_back(ch);
            }
        }
        std::cin.get();         // 处理回车
        std::cout << std::endl; // 换行

        // 登录锁
        if (shm->user_list[cur_label].is_login_prompt) {
            while (1) {
                Sleep(10);
                if (shm->user_list[cur_label].is_login_success || shm->user_list[cur_label].is_login_fail) {
                    break;
                }
            }
        }
        std::string login_info = user + " " + password + " " + std::to_string(cur_label);
        strncpy(shm->user_list[cur_label].command, login_info.c_str(), sizeof(shm->user_list[cur_label].command) - 1);
        shm->user_list[cur_label].cur_user = cur_label;
        shm->user_list[cur_label].is_login_prompt = true;

        while (true) {
            if (shm->user_list[cur_label].is_login_success) {
                break;
            } else if (shm->user_list[cur_label].is_login_fail) {
                std::cout << shm->user_list[cur_label].result;
                break;
            }
            Sleep(10);
        }
        if (shm->user_list[cur_label].is_login_success) {
            // 登录成功
            break;
        }
    }

    // 程序的进入页面
    std::cout << welcome1 << std::endl;
    std::cout << "键入 'help' 获得帮助" << std::endl;
    std::cout << "键入 'exit' 退出登录" << std::endl;
    std::cout << shm->user_list[cur_label].result;
    while (true) {
        // 读取结果并打印
        std::string command;
        std::getline(std::cin, command);
        if (command == "exit" || command == "EXIT") {
            // 清除用户信息
            shm->user_list[cur_label].is_login_success = false;
            shm->user_list[cur_label].is_login_fail = false;
            shm->user_list[cur_label].is_login_prompt = false;
            shm->user_list[cur_label].done = false;
            shm->user_list[cur_label].ready = false;
            shm->user_list[cur_label].cur_user = -1;
            shm->user_list[cur_label].cur_dir_inode_id = 0;
            break;
        }
        if (command == "help" || command == "HELP") {
            print_help((SharedMemory *)shm, cur_label);
            continue;
        }

        /***** 判断是否互斥   ********/
        std::string input, cmd, tmp_arg;
        std::vector<std::string> args;
        std::istringstream iss(command);
        iss >> cmd;
        while (iss >> tmp_arg) {
            args.push_back(tmp_arg);
        }
        // 解析参数
        std::map<std::string, std::string> options;
        std::string arg;
        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i].front() == '-') {
                // 检查是否有参数
                if (i + 1 < args.size() && args[i + 1].front() != '-') {
                    options[args[i]] = args[i + 1];
                    ++i; // 跳过参数
                } else {
                    options[args[i]] = "true";
                }
            } else {
                // 将最后一个非选项参数赋值给 arg
                arg = args[i];
            }
        }
        if (arg.empty() && !args.empty()) {
            arg = args.back();
        }
        // 确保arg是文件名
        if (arg.find("-") == std::string::npos) {
            // 将args分为文件名和路径
            size_t pos = arg.find_last_of('/');
            std::string file_path, file_name;
            if (pos != std::string::npos) {
                file_path = arg.substr(0, pos + 1);
                file_name = arg.substr(pos + 1);
            } else {
                file_path = "";
                file_name = arg;
            }
            if (arg.front() != '/') {
                file_path = get_absolute_path(shm->user_list[cur_label].cur_dir_inode_id) + file_path;
            }
            // 得到文件inode号，判断是否发生互斥
            uint32_t start_id = 0;
            if (is_dir_exit(file_path, start_id)) { // 目录存在
                Inode __dir_inode = Inode::read_inode(start_id);
                uint32_t file_id = get_file_inode_id(file_name, __dir_inode);
                if (cmd == "cat" || cmd == "copy" || cmd == "del" || cmd == "newfile") {
                    if (shm->open_file_table.is_writing(file_id)) {
                        std::cout << __ERROR << "文件" << file_name << "正在被写入" << __NORMAL << std::endl;

                        // 输出shm->result的最后一行
                        std::string result = shm->user_list[cur_label].result;
                        size_t last_newline = result.find_last_of('\n');
                        if (last_newline != std::string::npos) {
                            std::cout << result.substr(last_newline + 1);
                        } else {
                            std::cout << result;
                        }
                        continue;
                    }
                }
            }
        }
        strncpy(shm->user_list[cur_label].command, command.c_str(), sizeof(shm->user_list[cur_label].command) - 1);
        shm->user_list[cur_label].cur_user = cur_label;
        shm->user_list[cur_label].ready = true;

        if (command == "clear" || command == "cls" || command == "CLEAR" || command == "CLS")
            system("cls");
        if (command == "shutdown" || command == "SHUTDOWN")
            break;

        // 重置状态
        while (1) {
            Sleep(10);
            if (shm->user_list[cur_label].done) {
                break;
            }
        }
        std::cout << shm->user_list[cur_label].result;
        shm->user_list[cur_label].done = false;
    }

    // 清理资源
    UnmapViewOfFile(shm);
    CloseHandle(hMapFile);

    return 0;
}

void print_help(SharedMemory *shm, int cur_user) {
    std::cout << "命令列表:" << std::endl;
    std::cout << std::setfill('-') << std::setw(45) << "-" << std::setfill(' ') << std::endl;
    std::cout << __SUCCESS << std::left << std::setw(12) << "help: " << __NORMAL << "显示帮助信息" << std::endl;
    std::cout << __SUCCESS << std::left << std::setw(12) << "exit: " << __NORMAL << "退出登录" << std::endl;
    std::cout << __SUCCESS << std::left << std::setw(12) << "info: " << __NORMAL << "显示系统信息" << std::endl;
    std::cout << __SUCCESS << std::left << std::setw(12) << "cd: " << __NORMAL << "切换目录" << std::endl;
    std::cout << __SUCCESS << std::left << std::setw(12) << "md: " << __NORMAL << "创建目录" << std::endl;
    std::cout << __SUCCESS << std::left << std::setw(12) << "rd: " << __NORMAL << "删除目录" << std::endl;
    std::cout << __SUCCESS << std::left << std::setw(12) << "newfile: " << __NORMAL << "创建新文件" << std::endl;
    std::cout << __SUCCESS << std::left << std::setw(12) << "cat: " << __NORMAL << "显示文件内容或向文件追加内容" << std::endl;
    std::cout << __SUCCESS << std::left << std::setw(12) << "copy: " << __NORMAL << "复制文件" << std::endl;
    std::cout << __SUCCESS << std::left << std::setw(12) << "del: " << __NORMAL << "删除文件" << std::endl;
    std::cout << __SUCCESS << std::left << std::setw(12) << "check: " << __NORMAL << "检查文件或目录" << std::endl;
    std::cout << __SUCCESS << std::left << std::setw(12) << "dir|ls: " << __NORMAL << "显示目录内容" << std::endl;
    std::cout << __SUCCESS << std::left << std::setw(12) << "clear|cls: " << __NORMAL << "清空屏幕" << std::endl;
    std::cout << __SUCCESS << std::left << std::setw(12) << "adduser: " << __NORMAL << "添加用户" << std::endl;
    std::cout << __SUCCESS << std::left << std::setw(12) << "shutdown: " << __NORMAL << "退出登录并关闭系统" << std::endl;
    std::cout << __SUCCESS << std::left << std::setw(12) << "init: " << __NORMAL << "格式化磁盘" << std::endl;
    std::cout << "使用" << __SUCCESS << "<command> -h " << __NORMAL << "查看命令的具体使用方法" << std::endl;
    std::cout << "注意：命令不区分大小写" << std::endl;
    // 输出shm->result的最后一行
    std::string result = shm->user_list[cur_user].result;
    size_t last_newline = result.find_last_of('\n');
    if (last_newline != std::string::npos) {
        std::cout << result.substr(last_newline + 1);
    } else {
        std::cout << result;
    }
}