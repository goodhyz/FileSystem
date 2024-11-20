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
    shm->ready = false; // shell是否输入完毕
    shm->done = false;  // simdisk是否完成操作
    shm->is_login_prompt = false;
    shm->is_login_success = false;
    shm->is_login_fail = false;

    std::string shell_output = "";
    Inode root_inode, cur_inode;
    root_inode = cur_inode = Inode::read_inode(0);
    User user = User();
    SuperBlock sb = SuperBlock::read_super_block();      // 读超级块
    uint32_t load_time = static_cast<uint32_t>(time(0)); // 保留登录时间

    // 处理登录逻辑 多用户
    while (true) {
        if (shm->is_login_prompt) {
            // 读取账号密码
            std::istringstream iss(shm->command);
            std::string username, password;
            iss >> username >> password;
            // 判断账号密码正确性
            if (login(username, password, shell_output, user)) { // 正确
                shm->is_login_success = true;
                shm->is_login_prompt = false;
            } else { // 错误
                shm->is_login_fail = true;
                shm->is_login_prompt = false;
            }
        }
        strncpy(shm->result, shell_output.c_str(), sizeof(shm->result) - 1);
        if (shm->is_login_success) {
            break;
        }
        Sleep(10);
    }

    // 当前系统的信息
    std::string path = "/";
    shell_output = "上次登录时间: " + format_time(sb.last_load_time) + "\n";
    shell_output += __USER + user.username + "@FileSystem" + __NORMAL + ":" + __PATH + path + __NORMAL + "$ ";
    strncpy(shm->result, shell_output.c_str(), sizeof(shm->result) - 1);
    while (1) {
        if (shm->ready) {
            // 读取shell输入
            std::string input, cmd, tmp_arg;
            std::vector<std::string> args;
            input = shm->command;
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
            if (cmd == "exit" || cmd == "EXIT") {
                shm->done = true;
                shm->ready = false;
                break;
            } else if (cmd == "init" || cmd == "INIT") {
                init_disk();
                sb = SuperBlock::read_super_block();
            } else if (cmd == "info" || cmd == "INFO") {
                sb.save_super_block();
                shell_output = sb.print_super_block();
            } else if (cmd == "cd" || cmd == "CD") {
                uint32_t purpose_id = cur_inode.i_id;
                if (arg.empty()) { // no args ,into root
                    cur_inode = root_inode;
                    path = "/";
                    continue;
                }
                if (arg.back() != '/') {
                    arg += "/";
                }
                if (is_path_dir(arg, purpose_id, shell_output)) { // 是一个目录
                    cur_inode = Inode::read_inode(purpose_id);
                    path = get_absolute_path(purpose_id);
                }
            } else if (cmd == "md" || cmd == "MD") {
                while (1) {
                    // no args ,raise error
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
                    uint32_t start_id;
                    // 绝对路径
                    if (arg.front() == '/') {
                        start_id = 0;
                        if (is_dir_exit(arg, start_id)) {
                            std::cout << __ERROR << "目录" << arg << "已存在" << __NORMAL << std::endl;
                            shell_output += __ERROR + "目录" + arg + "已存在" + __NORMAL + "\n";
                        } else {
                            if (make_dir(arg, root_inode)) {
                                std::cout << __SUCCESS << "目录" << arg << "创建成功" << __NORMAL << std::endl;
                                shell_output += __SUCCESS + "目录" + arg + "创建成功" + __NORMAL + "\n";
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
                            if (make_dir(arg, cur_inode)) {
                                std::cout << __SUCCESS << "目录" << arg << "创建成功" << __NORMAL << std::endl;
                                shell_output += __SUCCESS + "目录" + arg + "创建成功" + __NORMAL + "\n";
                            }
                        }
                    }
                    break;
                }
            } else if (cmd == "rd" || cmd == "RD") {
                while (1) {
                    // no args ,raise error
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
                        if (is_dir_empty(purpose_id) || !options["-rf"].empty()) {
                            if (del_dir(purpose_id, shell_output)) {
                                std::cout << __SUCCESS << "目录" << dir_path << "删除成功" << __NORMAL << std::endl;
                                shell_output += __SUCCESS + "目录" + dir_path + "删除成功" + __NORMAL + "\n";
                                if (cur_inode.i_id == purpose_id) {
                                    cur_inode = root_inode;
                                    path = "/";
                                }
                            } else {
                                std::cout << __ERROR << "目录" << dir_path << "不为空" << __NORMAL << std::endl;
                                shell_output += __ERROR + "目录" + dir_path + "不为空" + __NORMAL + "\n";
                            }
                        } else {
                            std::cout << __ERROR << "目录" << dir_path << "不存在" << __NORMAL << std::endl;
                            shell_output += __ERROR + "目录" + dir_path + "不存在" + __NORMAL + "\n";
                        }
                    }
                    break;
                }
            } else if (cmd == "newfile" || cmd == "NEWFILE") {
                while (1) {
                    // no args ,raise error
                    if (arg.empty()) {
                        std::cout << __ERROR << "请输入文件名" << __NORMAL << std::endl;
                        shell_output += __ERROR + "请输入文件名" + __NORMAL + "\n";
                        break;
                    }
                    // args , analyse path
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
                        make_file(file_name, start_id, shell_output);
                    } else { // 目录不存在
                        if (make_dir(file_path, root_inode)) {
                            is_dir_exit(file_path, start_id); // 找到目录
                            make_file(file_name, start_id, shell_output);
                        }
                    }
                    shell_output += __SUCCESS + "文件" + file_name + "创建成功" + __NORMAL + "\n";
                    break;
                }
            } else if (cmd == "cat" || cmd == "CAT") {
                while (1) {
                    // no args ,raise error
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
                        // 读文件
                        if (options["-i"].empty()) {
                            std::string output = read_file(file_path, file_name);
                            if (!output.empty()) {
                                std::cout << output << std::endl;
                                shell_output += output + "\n";
                            }
                        } else { // -i
                            write_file(file_path, file_name, options["-i"]);
                            shell_output += __SUCCESS + "文件" + file_name + "写入成功" + __NORMAL + "\n";
                        }
                    }
                    break;
                }
            } else if (cmd == "copy" || cmd == "COPY") {
                while (1) {
                    // copy -r /a path
                    if (arg.empty()) {
                        std::cout << __ERROR << "请输入文件名" << __NORMAL << std::endl;
                        shell_output += __ERROR + "请输入文件名" + __NORMAL + "\n";
                        break;
                    }
                    // args , analyse path
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
                        // 读取文件系统的resource_path
                        file_content = read_file(__path, __name);
                    }
                    if (overwrite) { // 文件存在，覆盖
                        clear_file(target_path, target_name);
                        write_file(target_path, target_name, file_content);
                    } else {
                        if (!is_dir_exit(target_path, start_id)) { // 文件不存在
                            make_dir(target_path, root_inode);
                            is_dir_exit(target_path, start_id); // 找到目录id
                            make_file(target_name, start_id, shell_output);
                        }
                        if (!is_file_exit(target_name, dir_inode)) {
                            make_file(target_name, start_id, shell_output);
                        }
                        write_file(target_path, target_name, file_content);
                    }
                    std::cout << __SUCCESS << "文件" << target_name << "复制成功" << __NORMAL << std::endl;
                    shell_output += __SUCCESS + "文件" + target_name + "复制成功" + __NORMAL + "\n";
                    break;
                }
            } else if (cmd == "del" || cmd == "DEL") {
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
                        if (is_file_exit(file_name, dir_inode)) {
                            if (del_file(file_name, dir_inode, shell_output)) {
                                std::cout << __SUCCESS << "文件" << file_name << "删除成功" << __NORMAL << std::endl;
                                shell_output += __SUCCESS + "文件" + file_name + "删除成功" + __NORMAL + "\n";
                            } else {
                                std::cout << __ERROR << "文件" << file_name << "删除失败" << __NORMAL << std::endl;
                                shell_output += __ERROR + "文件" + file_name + "删除失败" + __NORMAL + "\n";
                            }
                        } else {
                            std::cout << __ERROR << "文件" << file_name << "不存在" << __NORMAL << std::endl;
                            shell_output += __ERROR + "文件" + file_name + "不存在" + __NORMAL + "\n";
                        }
                    }
                }

            } else if (cmd == "check" || cmd == "CHECK") {
            } else if (cmd == "DIR" || cmd == "dir" || cmd == "ls" || cmd == "LS") {
                shell_output = show_directory(cur_inode.i_id);
            } else if (cmd == "clear" || cmd == "CLEAR" || cmd == "cls" || cmd == "CLS") {
                system("cls");
            } else if (cmd == "adduser" || cmd == "ADDUSER") {
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
            } else {
                std::cout << __ERROR << "未定义的命令，请重新输入" << __NORMAL << std::endl;
                shell_output += __ERROR + "未定义的命令，请重新输入" + __NORMAL + "\n";
            }

            /*******  命令执行完后的操作  *******/
            shell_output += __USER + user.username + "@FileSystem" + __NORMAL + ":" + __PATH + path + __NORMAL + "$ ";
            strncpy(shm->result, shell_output.c_str(), sizeof(shm->result) - 1);
            shm->done = true;
            shm->ready = false;
        }
        Sleep(10); // 避免无用的循环
    }

    // 退出程序前保存超级块
    sb.last_load_time = load_time;
    sb.save_super_block();
    return 0;
}
