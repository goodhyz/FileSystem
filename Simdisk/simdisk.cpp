/**
 * @file simdisk.cpp
 * @brief 文件系统服务端程序
 * @author Hu Yuzhi
 * @date 2024-11-20
 */
#include "simdisk.h"

#include "share_memory.h"

// 全局变量
InodeBitmap inode_bitmap;
BlockBitmap block_bitmap;

// 服务端程序的逻辑
int main() {
    // 创建内存映射文件
    HANDLE hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedMemory), "SimdiskSharedMemory");
    if (hMapFile == NULL) {
        std::cerr << "Could not create file mapping object: " << GetLastError() << std::endl;
        return 1;
    }

    // 映射到进程的地址空间
    SharedMemory *shm = (SharedMemory *)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemory));
    if (shm == NULL) {
        std::cerr << "Could not map view of file: " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        return 1;
    }

    // 初始化共享内存状态
    for (int i = 0; i < 10; ++i) {
        // shm->user_list[i].user = User();

        shm->user_list[i].done = false;  // simdisk是否完成操作
        shm->user_list[i].ready = false; // shell是否输入完毕

        shm->user_list[i].is_login_prompt = false;
        shm->user_list[i].is_login_success = false;
        shm->user_list[i].is_login_fail = false;

        shm->user_list[i].cur_dir_inode_id = 0;
        shm->user_list[i].cur_user = -1;
        shm->open_file_table.opened_file[i].inode_id = -1;
        shm->open_file_table.opened_file[i].is_write = false;
    }

    std::string shell_output = "";
    Inode root_inode;
    root_inode = Inode::read_inode(0);
    SuperBlock sb = SuperBlock::read_super_block();      // 读超级块
    uint32_t load_time = static_cast<uint32_t>(time(0)); // 保留登录时间

    while (1) {

        // 确定是否有shell需要登录
        std::string user_label;
        for (int i = 0; i < 10; ++i) {
            if (shm->user_list[i].is_login_prompt) {
                // 读取账号密码
                std::istringstream iss(shm->user_list[i].command);
                std::string username, password;
                iss >> username >> password >> user_label;
                User user = shm->user_list[std::stoi(user_label)].user;
                // 判断账号密码正确性
                if (login(username, password, shell_output, user)) { // 正确
                    shm->user_list[i].is_login_success = true;
                    shm->user_list[i].is_login_prompt = false;
                } else { // 错误
                    shm->user_list[i].is_login_fail = true;
                    shm->user_list[i].is_login_prompt = false;
                }
                strncpy(shm->user_list[i].result, shell_output.c_str(), sizeof(shm->user_list[i].result) - 1);
                if (shm->user_list[i].is_login_success) {
                    shm->user_list[std::stoi(user_label)].user = user;
                    shell_output = __USER + user.username + "@FileSystem" + __NORMAL + ":" + __PATH + '/' + __NORMAL + "$ ";
                    strncpy(shm->user_list[i].result, shell_output.c_str(), sizeof(shm->user_list[i].result) - 1);
                    continue;
                }
            }
        }

        for (int i = 0; i < 10; ++i) {
            if (shm->user_list[i].ready) {
                // 确定指令的发起用户及其所在目录
                User user = shm->user_list[i].user;
                Inode cur_inode = Inode::read_inode(shm->user_list[i].cur_dir_inode_id);
                std::string path = get_absolute_path(cur_inode.i_id);
                // 读取shell输入
                std::string input, cmd, tmp_arg;
                std::vector<std::string> args;
                input = shm->user_list[i].command;
                std::istringstream iss(input);
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

                /*******处理命令********/
                shell_output = "";
                if (cmd == "shutdown" || cmd == "shutdown") {
                    if (options.find("-h") != options.end()) {
                        shell_output += "shutdown: 退出程序\n";
                        shell_output += "用法: exit\n";
                    } else {
                        shm->user_list[i].user = User();
                        shm->user_list[i].done = true;
                        shm->user_list[i].ready = false;
                        goto LABEL;
                        break;
                    }
                } else if (cmd == "init" || cmd == "INIT") {
                    if (options.find("-h") != options.end()) {
                        shell_output += "init: 初始化文件系统\n";
                        shell_output += "用法: init\n";
                    } else {
                        if (user.uid != 0) {
                            shell_output += __ERROR + "你没有权限初始化文件系统" + __NORMAL + "\n";
                        } else {
                            init_disk();
                            sb = SuperBlock::read_super_block();
                        }
                    }
                } else if (cmd == "info" || cmd == "INFO") {
                    if (options.find("-h") != options.end()) {
                        shell_output += "info: 显示文件系统信息\n";
                        shell_output += "用法: info\n";
                    } else {
                        sb.save_super_block();
                        shell_output = sb.print_super_block();
                        int user_count = 0;
                        for (int i = 0; i < 10; ++i) {
                            if (shm->user_list[i].is_login_success) {
                                ++user_count;
                            }
                        }
                        shell_output += "当前登录用户数: " + std::to_string(user_count) + "\n\n";
                    }
                } else if (cmd == "cd" || cmd == "CD") {
                    while (1) {
                        if (options.find("-h") != options.end()) {
                            shell_output += "cd: 切换目录\n";
                            shell_output += "用法: cd [path]\n";
                        } else {
                            uint32_t purpose_id = cur_inode.i_id;
                            if (arg.empty()) { // 没有参数 进入根目录
                                cur_inode = root_inode;
                                path = "/";
                                break;
                            }
                            if (arg.back() != '/') {
                                arg += "/";
                            }
                            if (is_path_dir(arg, purpose_id, shell_output)) { // 是一个目录
                                if (!is_able_to_execute(purpose_id, user)) {
                                    std::cout << __ERROR << "你没有权限访问" << arg << __NORMAL << std::endl;
                                    shell_output += __ERROR + "你没有权限访问" + arg + __NORMAL + "\n";
                                    break;
                                }
                                cur_inode = Inode::read_inode(purpose_id);
                                path = get_absolute_path(purpose_id);
                            }
                            break;
                        }
                    }
                } else if (cmd == "md" || cmd == "MD") {
                    if (options.find("-h") != options.end()) {
                        shell_output += "md: 创建目录\n";
                        shell_output += "用法: md <path> [-m <mode>]\n";
                        shell_output += "选项:\n";
                        shell_output += "  -m <mode>: 设置目录权限，默认为755\n";
                    } else {
                        uint32_t mode = 755;
                        while (1) {
                            if (options.find("-m") != options.end()) {
                                mode = std::stoi(options["-m"]);
                            }
                            // 没有参数 报错
                            if (arg.empty()) {
                                std::cout << __ERROR << "请输入目录名" << __NORMAL << std::endl;
                                shell_output += __ERROR + "请输入目录名" + __NORMAL + "\n";
                                break;
                            }
                            if (arg.back() != '/') {
                                arg += "/";
                            }
                            if (arg.find("//") != std::string::npos) {
                                std::cout << __ERROR << "输入路径格式错误，请重新输入" << __NORMAL << std::endl;
                                shell_output += __ERROR + "输入路径格式错误，请重新输入" + __NORMAL + "\n";
                                break;
                            }
                            uint32_t mode = 755;
                            if (options.find("-m") != options.end()) {
                                mode = std::stoi(options["-m"]);
                            }
                            uint32_t start_id;
                            // 绝对路径
                            if (arg.front() == '/') {
                                start_id = 0;
                                if (is_dir_exit(arg, start_id)) {
                                    std::cout << __ERROR << "目录" << arg << "已存在" << __NORMAL << std::endl;
                                    shell_output += __ERROR + "目录" + arg + "已存在" + __NORMAL + "\n";
                                } else {
                                    if (!is_able_to_write(start_id, user)) {
                                        std::cout << __ERROR << "你没有权限创建" << arg << __NORMAL << std::endl;
                                        shell_output += __ERROR + "你没有权限创建" + arg + __NORMAL + "\n";
                                    } else {
                                        if (make_dir(arg, root_inode, user, shell_output,mode)) {
                                            std::cout << __SUCCESS << "目录" << arg << "创建成功" << __NORMAL << std::endl;
                                            shell_output += __SUCCESS + "目录" + arg + "创建成功" + __NORMAL + "\n";
                                        }
                                    }
                                }
                            }
                            // 相对路径
                            else {
                                start_id = cur_inode.i_id;
                                if (is_dir_exit(arg, start_id)) {
                                    std::cout << __ERROR << "目录" << arg << "已存在" << __NORMAL << std::endl;
                                    shell_output += __ERROR + "目录" + arg + "已存在" + __NORMAL + "\n";
                                } else {
                                    if (!is_able_to_write(start_id, user)) {
                                        std::cout << __ERROR << "你没有权限创建" << arg << __NORMAL << std::endl;
                                        shell_output += __ERROR + "你没有权限创建" + arg + __NORMAL + "\n";
                                    } else {
                                        if (make_dir(arg, cur_inode, user,shell_output, mode)) {
                                            std::cout << __SUCCESS << "目录" << arg << "创建成功" << __NORMAL << std::endl;
                                            shell_output += __SUCCESS + "目录" + arg + "创建成功" + __NORMAL + "\n";
                                        }
                                    }
                                }
                            }
                            break;
                        }
                    }
                } else if (cmd == "rd" || cmd == "RD") {
                    if (options.find("-h") != options.end()) {
                        shell_output += "rd: 删除目录\n";
                        shell_output += "用法: rd [-rf] <path>\n";
                        shell_output += "Options:\n";
                        shell_output += "  -rf: 强制递归删除目录\n";
                    } else {
                        while (1) {
                            // 没有参数
                            if (arg.empty()) {
                                std::cout << __ERROR << "请输入目录名" << __NORMAL << std::endl;
                                shell_output += __ERROR + "请输入目录名" + __NORMAL + "\n";
                                break;
                            }
                            if (arg.find("//") != std::string::npos) {
                                std::cout << __ERROR << "输入路径格式错误，请重新输入" << __NORMAL << std::endl;
                                shell_output += __ERROR + "输入路径格式错误，请重新输入" + __NORMAL + "\n";
                                break;
                            }
                            if (arg.back() != '/') {
                                arg += "/";
                            }
                            // 全部转换为绝对路径
                            std::string dir_path = arg;
                            uint32_t purpose_id = 0;
                            if (dir_path.front() != '/') {
                                dir_path = get_absolute_path(cur_inode.i_id) + dir_path;
                            }
                            if (is_dir_exit(dir_path, purpose_id)) {
                                if (!is_able_to_write(purpose_id, user)) {
                                    std::cout << __ERROR << "你没有权限删除" << dir_path << __NORMAL << std::endl;
                                    shell_output += __ERROR + "你没有权限删除" + dir_path + __NORMAL + "\n";
                                    break;
                                }
                                if (is_dir_empty(purpose_id) || !options["-rf"].empty()) {
                                    if (del_dir(purpose_id, shell_output)) {
                                        std::cout << __SUCCESS << "目录" << dir_path << "删除成功" << __NORMAL << std::endl;
                                        shell_output += __SUCCESS + "目录" + dir_path + "删除成功" + __NORMAL + "\n";
                                        if (cur_inode.i_id == purpose_id) {
                                            cur_inode = root_inode;
                                            path = "/";
                                        }
                                    }
                                } else {
                                    std::cout << __ERROR << "目录" << dir_path << "不为空" << __NORMAL << std::endl;
                                    shell_output += __ERROR + "目录" + dir_path + "不为空" + __NORMAL + "\n";
                                }
                            } else {
                                std::cout << __ERROR << "目录" << dir_path << "不存在" << __NORMAL << std::endl;
                                shell_output += __ERROR + "目录" + dir_path + "不存在" + __NORMAL + "\n";
                            }
                            break;
                        }
                    }
                } else if (cmd == "newfile" || cmd == "NEWFILE") {
                    uint32_t mode = 755;
                    if (options.find("-h") != options.end()) {
                        shell_output += "newfile: 创建新文件\n";
                        shell_output += "用法: newfile <path> [-m <mode>]\n";
                        shell_output += "选项:\n";
                        shell_output += "  -m <mode>: 设置文件权限，默认为755\n";
                    } else {
                        while (1) {
                            if (options.find("-m") != options.end()) {
                                mode = std::stoi(options["-m"]);
                            }
                            // 没有参数
                            if (arg.empty()) {
                                std::cout << __ERROR << "请输入文件名" << __NORMAL << std::endl;
                                shell_output += __ERROR + "请输入文件名" + __NORMAL + "\n";
                                break;
                            }
                            // 解析参数
                            if (arg.find("//") != std::string::npos || arg.back() == '/') {
                                std::cout << __ERROR << "文件名输入错误，请重新输入" << __NORMAL << std::endl;
                                shell_output += __ERROR + "文件名输入错误，请重新输入" + __NORMAL + "\n";
                                break;
                            }
                            uint32_t start_id = 0;
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

                                file_path = get_absolute_path(cur_inode.i_id) + file_path;
                            }
                            if (is_dir_exit(file_path, start_id)) { // 目录存在
                                if (!is_able_to_write(start_id, user)) {
                                    std::cout << __ERROR << "你没有权限创建" << arg << __NORMAL << std::endl;
                                    shell_output += __ERROR + "你没有权限创建" + arg + __NORMAL + "\n";
                                    break;
                                }
                                if(!make_file(file_name, start_id,  user,shell_output, mode))
                                {
                                    std::cout << __ERROR << "文件" << file_name << "创建失败" << __NORMAL << std::endl;
                                    shell_output += __ERROR + "文件" + file_name + "创建失败" + __NORMAL + "\n";
                                    break;
                                }   
                                Inode __dir_inode = Inode::read_inode(start_id);
                                uint32_t file_id = get_file_inode_id(file_name, __dir_inode);
                                shm->open_file_table.add_file(file_id, true);
                                Sleep(5000);
                                shm->open_file_table.close_file(file_id);
                            } else { // 目录不存在
                                if (!is_able_to_write(start_id, user)) {
                                    std::cout << __ERROR << "你没有权限创建" << arg << __NORMAL << std::endl;
                                    shell_output += __ERROR + "你没有权限创建" + arg + __NORMAL + "\n";
                                    break;
                                }
                                if (make_dir(file_path, root_inode, user,shell_output, mode)) {
                                    is_dir_exit(file_path, start_id); // 找到目录
                                    
                                    if(!make_file(file_name, start_id, user,shell_output, mode)){
                                        std::cout << __ERROR << "文件" << file_name << "创建失败" << __NORMAL << std::endl;
                                        shell_output += __ERROR + "文件" + file_name + "创建失败" + __NORMAL + "\n";
                                        break;
                                    }
                                    Inode __dir_inode = Inode::read_inode(start_id);
                                    uint32_t file_id = get_file_inode_id(file_name, __dir_inode);
                                    shm->open_file_table.add_file(file_id, true);
                                    Sleep(5000);
                                    shm->open_file_table.close_file(file_id);
                                }
                            }
                            shell_output += __SUCCESS + "文件" + file_name + "创建成功" + __NORMAL + "\n";
                            break;
                        }
                    }
                } else if (cmd == "cat" || cmd == "CAT") {
                    if (options.find("-h") != options.end()) {
                        shell_output += "cat: 显示文件内容\n";
                        shell_output += "用法: cat [-i <content>] <filename>\n";
                        shell_output += "选项:\n";
                        shell_output += "  -i <content>: 向文件追加内容\n";
                    } else {
                        while (1) {
                            // 没有参数
                            if (arg.empty()) {
                                std::cout << __ERROR << "请输入文件名" << __NORMAL << std::endl;
                                shell_output += __ERROR + "请输入文件名" + __NORMAL + "\n";
                                break;
                            }
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
                                file_path = get_absolute_path(cur_inode.i_id) + file_path;
                            }
                            uint32_t start_id = 0;
                            is_dir_exit(file_path, start_id);
                            if (!is_file_exit(file_name, Inode::read_inode(start_id))) {
                                std::cout << __ERROR << "文件" << file_name << "不存在" << __NORMAL << std::endl;
                                shell_output += __ERROR + "文件" + file_name + "不存在" + __NORMAL + "\n";
                                break;
                            } else {
                                Inode file_inode = Inode::read_inode(start_id);
                                // 读文件
                                if (options["-i"].empty()) {
                                    if (!is_able_to_read(get_file_inode_id(file_name, cur_inode), user)) {
                                        std::cout << __ERROR << "你没有权限读取" << file_name << __NORMAL << std::endl;
                                        shell_output += __ERROR + "你没有权限读取" + file_name + __NORMAL + "\n";
                                        break;
                                    }
                                    std::string output = read_file(file_path, file_name);
                                    if (!output.empty()) {
                                        std::cout << output << std::endl;
                                        shell_output += output + "\n";
                                    }
                                } else { // -i
                                    int file_id = get_file_inode_id(file_name, cur_inode);
                                    if (!is_able_to_write(file_id, user)) {
                                        std::cout << __ERROR << "你没有权限写入" << file_name << __NORMAL << std::endl;
                                        shell_output += __ERROR + "你没有权限写入" + file_name + __NORMAL + "\n";
                                        break;
                                    }
                                    if (shm->open_file_table.is_writing(file_id)) {
                                        std::cout << __ERROR << "文件" << file_name << "正在被写入" << __NORMAL << std::endl;
                                        shell_output += __ERROR + "文件" + file_name + "正在被写入" + __NORMAL + "\n";
                                        break;
                                    } else {
                                        shm->open_file_table.add_file(file_id, true);
                                        Sleep(5000);
                                        std::cout<<"writing"<<file_name<<std::endl;
                                    }
                                    write_file(file_path, file_name, options["-i"]);
                                    shell_output += __SUCCESS + "文件" + file_name + "写入成功" + __NORMAL + "\n";
                                    shm->open_file_table.close_file(file_id);
                                }
                            }
                            break;
                        }
                    }
                } else if (cmd == "copy" || cmd == "COPY") {
                    uint32_t mode = 755;
                    if (options.find("-h") != options.end()) {
                        shell_output += "copy: 复制文件\n";
                        shell_output += "用法: copy <-host <host_path> | -fs <fs_path>> [-f] <filename> [-m <mode>]\n";
                        shell_output += "选项:\n";
                        shell_output += "  -host <host_path>: 指定宿主机的文件路径\n";
                        shell_output += "  -fs <fs_path>: 指定文件系统的文件路径\n";
                        shell_output += "  -f: 强制覆盖目标文件\n";
                        shell_output += "  -m <mode>: 设置文件权限，默认为755\n";
                    } else {
                        while (1) {
                            uint32_t mode = 755;
                            if (options.find("-m") != options.end()) {
                                mode = std::stoi(options["-m"]);
                            }
                            // 处理没有参数的情况
                            if (arg.empty()) {
                                std::cout << __ERROR << "请输入文件名" << __NORMAL << std::endl;
                                shell_output += __ERROR + "请输入文件名" + __NORMAL + "\n";
                                break;
                            }
                            // 分析参数
                            if (arg.find("//") != std::string::npos || arg.back() == '/') {
                                std::cout << __ERROR << "文件名输入错误，请重新输入" << __NORMAL << std::endl;
                                shell_output += __ERROR + "文件名输入错误，请重新输入" + __NORMAL + "\n";
                                break;
                            }
                            std::string resource_path, target_path, target_name;
                            uint32_t start_id = 0;
                            // 将args分为文件名和路径
                            size_t pos = arg.find_last_of('/');
                            if (pos != std::string::npos) {
                                target_path = arg.substr(0, pos + 1);
                                target_name = arg.substr(pos + 1);
                            } else {
                                target_path = "";
                                target_name = arg;
                            }
                            if (arg.front() != '/') {
                                target_path = get_absolute_path(cur_inode.i_id) + target_path;
                            }
                            bool overwrite = false;
                            if (is_dir_exit(target_path, start_id) && is_file_exit(target_name, Inode::read_inode(start_id)) && options["-f"].empty()) {
                                std::cout << "目标文件" << target_name << "已存在" << std::endl;
                                shell_output += "目标文件" + target_name + "已存在\n";
                                break;
                            } else if (is_dir_exit(target_path, start_id) && is_file_exit(target_name, Inode::read_inode(start_id)) && !options["-f"].empty()) {
                                overwrite = true;
                            }
                            Inode dir_inode = Inode::read_inode(start_id);
                            std::string file_content;
                            if (options["-host"].empty() && options["-fs"].empty()) {
                                std::cout << __ERROR << "请指定源文件的系统" << __NORMAL << std::endl;
                                shell_output += __ERROR + "请指定源文件的系统" + __NORMAL + "\n";
                                break;
                            } else if (!options["-host"].empty()) {
                                resource_path = options["-host"];
                                // 读取系统文件的resource_path
                                std::fstream resource_file(resource_path, std::ios::in | std::ios::binary);
                                if (!resource_file.is_open()) {
                                    std::cout << __ERROR << "文件" << resource_path << "不存在" << __NORMAL << std::endl;
                                    shell_output += __ERROR + "文件" + resource_path + "不存在" + __NORMAL + "\n";
                                    break;
                                }
                                // 读取文件内容
                                file_content = std::string((std::istreambuf_iterator<char>(resource_file)), std::istreambuf_iterator<char>());
                                resource_file.close();
                            } else {
                                resource_path = options["-fs"];
                                if (resource_path.find("//") != std::string::npos || resource_path.back() == '/') {
                                    std::cout << __ERROR << "输入路径格式错误，请重新输入" << __NORMAL << std::endl;
                                    shell_output += __ERROR + "输入路径格式错误，请重新输入" + __NORMAL + "\n";
                                    break;
                                }
                                if (resource_path.front() != '/') {
                                    resource_path = get_absolute_path(cur_inode.i_id) + resource_path;
                                }
                                std::string __path, __name;
                                size_t __pos = resource_path.find_last_of('/');
                                if (__pos != std::string::npos) {
                                    __path = resource_path.substr(0, __pos + 1);
                                    __name = resource_path.substr(__pos + 1);
                                } else {
                                    __path = "";
                                    __name = resource_path;
                                }
                                uint32_t __start_id = 0;
                                is_dir_exit(__path, __start_id);
                                Inode __cur_dir = Inode::read_inode(__start_id);
                                if (!is_able_to_read(get_file_inode_id(__name, __cur_dir), user)) {
                                    std::cout << __ERROR << "你没有权限读取" << __name << __NORMAL << std::endl;
                                    shell_output += __ERROR + "你没有权限读取" + __name + __NORMAL + "\n";
                                    break;
                                }
                                // 读取文件系统的resource_path
                                file_content = read_file(__path, __name);
                                if (__SUCCESS + "it is empty" == file_content) {
                                    file_content = "";
                                }
                            }
                            if (overwrite) { // 文件存在，覆盖
                                if (!is_able_to_write(start_id, user)) {
                                    std::cout << __ERROR << "你没有权限写入" << target_name << __NORMAL << std::endl;
                                    shell_output += __ERROR + "你没有权限写入" + target_name + __NORMAL + "\n";
                                    break;
                                }
                                Inode file_inode = Inode::read_inode(start_id);
                                uint32_t file_id = get_file_inode_id(target_name, file_inode);
                                if (shm->open_file_table.is_writing(file_id)) {
                                    std::cout << __ERROR << "文件" << target_name << "正在被写入" << __NORMAL << std::endl;
                                    shell_output += __ERROR + "文件" + target_name + "正在被写入" + __NORMAL + "\n";
                                    break;
                                } else {
                                    shm->open_file_table.add_file(file_id, true);
                                    Sleep(5000);
                                }
                                clear_file(target_path, target_name);
                                write_file(target_path, target_name, file_content);
                                shm->open_file_table.close_file(file_id);
                            } else { // 文件不存在
                                if (!is_able_to_write(start_id, user)) {
                                    std::cout << __ERROR << "你没有权限写入" << target_name << __NORMAL << std::endl;
                                    shell_output += __ERROR + "你没有权限写入" + target_name + __NORMAL + "\n";
                                    break;
                                }
                                if (!is_dir_exit(target_path, start_id)) { // 目录不存在
                                    make_dir(target_path, root_inode, user, shell_output,mode);
                                    is_dir_exit(target_path, start_id); // 找到目录id
                                    make_file(target_name, start_id,  user, shell_output,mode);
                                } else if (!is_file_exit(target_name, dir_inode)) {
                                    make_file(target_name, start_id,  user, shell_output,mode);
                                }
                                write_file(target_path, target_name, file_content);
                            }
                            // Sleep(10000);
                            std::cout << __SUCCESS << "文件" << target_name << "复制成功" << __NORMAL << std::endl;
                            shell_output += __SUCCESS + "文件" + target_name + "复制成功" + __NORMAL + "\n";
                            break;
                        }
                    }
                } else if (cmd == "del" || cmd == "DEL") {
                    if (options.find("-h") != options.end()) {
                        shell_output += "del: 删除文件\n";
                        shell_output += "用法: del <filename>\n";
                    } else {
                        // 没有参数
                        if (arg.empty()) {
                            std::cout << __ERROR << "请输入文件名" << __NORMAL << std::endl;
                            shell_output += __ERROR + "请输入文件名" + __NORMAL + "\n";
                        } else if (arg.find("//") != std::string::npos || arg.back() == '/') {
                            std::cout << __ERROR << "文件名输入错误，请重新输入" << __NORMAL << std::endl;
                            shell_output += __ERROR + "文件名输入错误，请重新输入" + __NORMAL + "\n";
                        } else {
                            uint32_t start_id = 0;
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
                                file_path = get_absolute_path(cur_inode.i_id) + file_path;
                            }
                            if (is_dir_exit(file_path, start_id)) { // 目录存在
                                Inode dir_inode = Inode::read_inode(start_id);
                                uint32_t file_id = get_file_inode_id(file_name, dir_inode);
                                if (shm->open_file_table.is_writing(file_id)) {
                                    std::cout << __ERROR << "文件" << file_name << "正在被写入" << __NORMAL << std::endl;
                                    shell_output += __ERROR + "文件" + file_name + "正在被写入" + __NORMAL + "\n";
                                    break;
                                } else {
                                    shm->open_file_table.add_file(file_id, true);
                                    Sleep(5000);
                                }
                                if (is_file_exit(file_name, dir_inode)) {
                                    if (!is_able_to_write(get_file_inode_id(file_name, dir_inode), user)) {
                                        std::cout << __ERROR << "你没有权限删除" << file_name << __NORMAL << std::endl;
                                        shell_output += __ERROR + "你没有权限删除" + file_name + __NORMAL + "\n";
                                    } else if (del_file(file_name, dir_inode, shell_output)) {
                                        std::cout << __SUCCESS << "文件" << file_name << "删除成功" << __NORMAL << std::endl;
                                        shell_output += __SUCCESS + "文件" + file_name + "删除成功" + __NORMAL + "\n";
                                        shm->open_file_table.close_file(file_id);
                                    } else {
                                        std::cout << __ERROR << "文件" << file_name << "删除失败" << __NORMAL << std::endl;
                                        shell_output += __ERROR + "文件" + file_name + "删除失败" + __NORMAL + "\n";
                                    }
                                } else {
                                    std::cout << __ERROR << "文件" << file_name << "不存在" << __NORMAL << std::endl;
                                    shell_output += __ERROR + "文件" + file_name + "不存在" + __NORMAL + "\n";
                                    shm->open_file_table.close_file(file_id);
                                }
                            }
                        }
                    }
                } else if (cmd == "check" || cmd == "CHECK") {
                    if (options.find("-h") != options.end()) {
                        shell_output += "check: 检查文件系统\n";
                        shell_output += "用法: check\n";
                    } else {
                        sb.save_super_block();
                        uint32_t used_block = block_bitmap.bitmap.count();
                        uint32_t used_inode = inode_bitmap.bitmap.count();
                        if (used_block == sb.block_count - sb.free_blocks && used_inode == sb.inode_count - sb.free_inodes) {
                            std::cout << __SUCCESS << "文件系统完好" << __NORMAL << std::endl;
                            shell_output += __SUCCESS + "文件系统完好" + __NORMAL + "\n";
                        } else {
                            std::cout << __ERROR << "文件系统损坏" << __NORMAL << std::endl;
                            shell_output += __ERROR + "文件系统损坏" + __NORMAL + "\n";
                        }
                    }
                } else if (cmd == "DIR" || cmd == "dir" || cmd == "ls" || cmd == "LS") {
                    if (options.find("-h") != options.end()) {
                        shell_output += "dir: 显示当前目录内容\n";
                        shell_output += "用法: dir [-s]\n";
                        shell_output += "选项:\n";
                        shell_output += "  -s: 递归显示\n";
                    } else {
                        if (!is_able_to_read(cur_inode.i_id, user)) {
                            std::cout << __ERROR << "你没有权限读取" << path << __NORMAL << std::endl;
                            shell_output += __ERROR + "你没有权限读取" + path + __NORMAL + "\n";
                        } else if (options["-s"].empty()) {
                            shell_output = show_directory(cur_inode.i_id, user, false);
                        } else {
                            shell_output = show_directory(cur_inode.i_id, user, true);
                        }
                    }
                } else if (cmd == "clear" || cmd == "CLEAR" || cmd == "cls" || cmd == "CLS") {
                    if (options.find("-h") != options.end()) {
                        shell_output += "clear: 清屏\n";
                        shell_output += "用法: clear\n";
                    } else {
                        system("cls");
                    }
                } else if (cmd == "adduser" || cmd == "ADDUSER") {
                    if (options.find("-h") != options.end()) {
                        shell_output += "adduser: 添加用户\n";
                        shell_output += "用法: adduser -u <username> -p <password> -uid <uid> -gid <gid>\n";
                    } else {
                        while (1) {
                            if (user.gid != 0) {
                                shell_output += __ERROR + "您没有权限使用,请联系管理员" + __NORMAL + "\n";
                                break;
                            }
                            if (options["-u"].empty() || options["-p"].empty() || options["-uid"].empty() || options["-gid"].empty()) {
                                shell_output += __ERROR + "请输入正确的参数" + __NORMAL + "\n";
                                break;
                            }
                            if (adduser(options["-u"], options["-p"], std::stoul(options["-uid"]), std::stoul(options["-gid"]))) {
                                shell_output += __SUCCESS + "用户" + options["-u"] + "创建成功" + __NORMAL + "\n";
                            } else {
                                shell_output += __ERROR + "用户" + options["-u"] + "创建失败" + __NORMAL + "\n";
                            }
                            break;
                        }
                    }
                } else {
                    std::cout << __ERROR << "未定义的命令，请重新输入" << __NORMAL << std::endl;
                    shell_output += __ERROR + "未定义的命令，请重新输入" + __NORMAL + "\n";
                }

                /*******  命令执行完后的操作  *******/
                shell_output += __USER + user.username + "@FileSystem" + __NORMAL + ":" + __PATH + path + __NORMAL + "$ ";
                strncpy(shm->user_list[i].result, shell_output.c_str(), sizeof(shm->user_list[i].result) - 1);
                shm->user_list[i].cur_dir_inode_id = cur_inode.i_id;
                shm->user_list[i].done = true;
                shm->user_list[i].ready = false;
            }
            Sleep(10); // 减少cpu占用
        }
    }

    LABEL:
    // 退出程序前保存超级块
    sb.last_load_time = load_time;
    sb.save_super_block();
    return 0;
}
