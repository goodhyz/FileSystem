/*
调试用的代码
*/
#include "simdisk.h"
#include <bitset>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
void print_block(const std::string &filename, std::streampos block_num) {
    std::ifstream file(filename, std::ios::binary);
    file.seekg(block_num * BLOCK_SIZE);
    char buffer[BLOCK_SIZE];
    file.read(buffer, BLOCK_SIZE);
    for (int i = 0; i < BLOCK_SIZE; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (static_cast<unsigned int>(buffer[i]) & 0xFF) << " ";
        if ((i + 1) % 16 == 0) {
            std::cout << std::endl;
        }
    }
    std::cout << std::dec; // 恢复为十进制格式
    file.close();
}
// 第一块（idx=0）作为超级块，数据块位图和inode位图。
// 一共102400个数据块，分配13个块（idx =1到 idx=13)作为数据块位图
// idx=16到idx=579表示inode相关数据
// idx=14到idx=15为inode位图
// idx=580到idx=599置0，便于扩展
// idx>=600块其为存储信息的数据块

std::string read_file(string path) {
    
    // 打开文件
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("无法打开文件: " + path);
    }

    // 读取文件内容
    std::ostringstream oss;
    oss << file.rdbuf();
    
    // 关闭文件
    file.close();
    
    // 返回文件内容作为字符串
    return oss.str();
}

// int main() {
//         print_block(disk_path, 600);
//         cout << static_cast<uint32_t>(-1) << endl;
//         return 0;
// }
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <string>

int main() {
    std::string path = "/";
    while (1) {
        string input, cmd, tmp_arg;
        cout << path << ">";
        vector<string> args;
        std::getline(cin, input);
        std::istringstream iss(input);
        iss >> cmd;
        while (iss >> tmp_arg) {
            args.push_back(tmp_arg);
        }

        // 解析选项
        std::map<std::string, std::string> options;
        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i].front() == '-') {
                if (i + 1 < args.size() && args[i + 1].front() != '-') {
                    options[args[i]] = args[i + 1];
                    ++i;
                } else {
                    options[args[i]] = "";
                }
            }
        }

        if (cmd == "cd") {
            // 处理cd命令
        } else if (cmd == "rd") {
            // 处理rd命令
            if (options.find("-rf") != options.end()) {
                // 处理 -rf 选项
                cout << "选项 -rf 被检测到" << endl;
            }
        }
    }
    return 0;
}