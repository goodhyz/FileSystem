/*
调试用的代码
*/
#include <iostream>
#include <fstream>
#include <cstdint>
#include <ctime>
#include <string>
#include <cstring>
#include <bitset>
#include <vector>
#include <iomanip>
#include "simdisk.h"

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
int main(){
    print_block(disk_path, 16);
    cout<<static_cast<uint32_t>(-1)<<endl;
    return 0;
}