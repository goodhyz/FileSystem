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

int main() {
    print_block(disk_path, 0);
    cout << static_cast<uint32_t>(-1) << endl;
    return 0;

}