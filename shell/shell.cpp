#include "share_memory.h"
#include "shell.h"

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
    int cur_user, cur_dir_inode_id = 0;

    while (true) {
        shm->is_login_fail = shm->is_login_success = false;
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
        cur_user = shm->get_free_user();
        if (cur_user == -1) {
            std::cerr << __ERROR << "用户数量已达上限" << __NORMAL << std::endl;
            return 1;
        }
        // 登录锁
        if(shm->is_login_prompt){
            while(1){
                Sleep(100);
                if(shm->is_login_success || shm->is_login_fail){
                    break;
                }
            }
        }
        std::string login_info = user + " " + password + " " + std::to_string(cur_user);
        strncpy(shm->command, login_info.c_str(), sizeof(shm->command) - 1);
        shm->cur_user = cur_user;
        shm->is_login_prompt = true;

        while (true) {
            if (shm->is_login_success) {
                break;
            } else if (shm->is_login_fail) {
                std::cout << shm->result;
                break;
            }
            Sleep(10);
        }
        if (shm->is_login_success) {
            // 登录成功
            break;
        }
    }

    // 程序的进入页面
    std::cout << welcome1 << std::endl;
    std::cout << "I am user " << cur_user << std::endl;
    std::cout << "键入 'help' 获得帮助" << std::endl;
    std::cout << "键入 'exit' 停止程序" << std::endl;
    std::cout << shm->result;
    while (true) {
        // 读取结果并打印
        std::string command;
        std::getline(std::cin, command);
        if (command == "exit")
            break;
        if (command == "help") {
            std::cout << "命令列表:" << std::endl;
            std::cout << "    help: 显示帮助信息" << std::endl;
            std::cout << "    exit: 退出程序" << std::endl;
            std::cout << "    info: 显示系统信息" << std::endl;
            std::cout << "    cd: 切换目录" << std::endl;
            std::cout << "    md: 创建目录" << std::endl;
            std::cout << "    rd: 删除目录" << std::endl;
            std::cout << "    newfile: 创建新文件" << std::endl;
            std::cout << "    cat: 显示文件内容或向文件追加内容" << std::endl;
            std::cout << "    copy: 复制文件" << std::endl;
            std::cout << "    del: 删除文件" << std::endl;
            std::cout << "    check: 检查文件或目录" << std::endl;
            std::cout << "    dir | ls: 显示目录内容" << std::endl;
            std::cout << "    clear | cls: 清空屏幕" << std::endl;
            std::cout << "    adduser: 添加用户" << std::endl;
            std::cout << "<command> -h 查看具体使用情况" << std::endl;
            // 输出shm->result的最后一行
            std::string result = shm->result;
            size_t last_newline = result.find_last_of('\n');
            if (last_newline != std::string::npos) {
                std::cout << result.substr(last_newline + 1);
            } else {
                std::cout << result;
            }
            continue;
        }
        if(shm->ready == true){
            std::cout<<"其他用户正在写，请稍后"<<std::endl;
            while(1){
                Sleep(100);
                if(shm->done){
                    break;
                }
            }
        }
        // 写入命令并通知后台
        strncpy(shm->command, command.c_str(), sizeof(shm->command) - 1);
        shm->cur_user = cur_user;
        shm->cur_dir_inode_id = cur_dir_inode_id;
        shm->ready = true;


        if (command == "clear")
            system("cls");

        // 重置状态
        while(1){
            Sleep(10);
            if(shm->done){
                break;
            }
        }
        std::cout << shm->result;
        cur_dir_inode_id = shm->cur_dir_inode_id;
        shm->done = false;
    }

    // 清理资源
    UnmapViewOfFile(shm);
    CloseHandle(hMapFile);

    return 0;
}
