#include <iostream>
#include <fstream>
#include <cstdint>
#include <ctime>
#include <string>
#include <cstring>
#include <bitset>
#include <vector>
#include <simdisk.h>

// 定义超级块结构体, 作为第一个块
struct SuperBlock
{
    uint32_t fs_size;            // 文件系统大小 （字节）
    uint32_t block_size;         // 块大小（字节）
    uint32_t inode_count;        // inode 总数
    uint32_t block_count;        // 数据块总数
    uint32_t free_inodes;        // 空闲 inode 数量
    uint32_t free_blocks;        // 空闲数据块数量
    uint32_t inode_bitmap_start; // inode 位图的起始位置
    uint32_t block_bitmap_start; // 数据块位图的起始位置
    uint32_t inode_list_start;   // inode 列表的起始位置
    uint32_t data_block_start;   // 数据块区域的起始位置

    // 成员函数：将超级块写入文件
    void writeToFile(const std::string &filename = disk_path, std::streampos block_num = 0) const
    {
        std::ofstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
        if (!file)
        {
            std::cerr << "Error opening file for writing: " << filename << std::endl;
            return;
        }
        file.seekp(block_num * BLOCK_SIZE);
        file.write(reinterpret_cast<const char *>(this), sizeof(SuperBlock));
        file.close();
    }

    // 成员函数：从文件读取超级块
    static SuperBlock readFromFile(const std::string &filename = disk_path, std::streampos block_num = 0)
    {
        SuperBlock sb;
        std::ifstream ifs(filename, std::ios::binary);
        if (!ifs)
        {
            std::cerr << "Error opening file for reading: " << filename << std::endl;
            return sb;
        }
        ifs.seekg(block_num * BLOCK_SIZE);
        ifs.read(reinterpret_cast<char *>(&sb), sizeof(SuperBlock));
        ifs.close();
        return sb;
    }
};

// 定义 inode 结构体
// inode 结构体大小为 48 字节，分配564块做inode表，2块做inode位图
struct Inode
{
    // 考虑对齐与填充，类型都设置为uint32_t
    uint32_t i_id;          // inode 编号, 表示为位置
    uint32_t i_size;        // 文件大小, 以字节为单位
    uint32_t i_blocks;      // 文件所占数据块数量
    uint32_t i_links_count; // 链接数
    uint32_t i_indirect;    // 一级间接块指针 0-102399
    uint32_t i_flags;       // 文件系统标志

    /*访问控制*/
    uint32_t i_mode; // 文件权限和类型 9位权限 + 3位文件类型
    uint32_t i_uid;  // 用户 ID
    uint32_t i_gid;  // 组 ID
    /*时间戳*/
    uint32_t i_ctime; // 创建时间
    uint32_t i_mtime; // 修改时间
    uint32_t i_atime; // 访问时间
};

// 定义目录项结构体, 32字节
struct DirEntry
{
    uint16_t inode_id; // inode < 12032
    uint16_t type;     // 文件类型 0-目录文件 1-普通文件 2-符号链接文件
    char name[28];     // 文件名
    void set(uint16_t inode_id, uint16_t type, const char *name)
    {
        this->inode_id = inode_id;
        this->type = type;
        strcpy(this->name, name);
    }
};

// 目录数据块
struct DirBlock
{
    DirEntry entries[32];
    void init_DirBlock(uint32_t parent_inode_id, uint32_t self_inode_id)
    {
        entries[0].set(self_inode_id, 0, ".");    // 当前目录
        entries[1].set(parent_inode_id, 0, ".."); // 父目录,如何得到父目录的inode_id？设置一个当前目录吗?
    }
};

// inode位图
struct InodeBitmap
{
    std::bitset<INODE_COUNT> bitmap;
    void init_bitmap()
    {
        bitmap.reset();
        bitmap.set(0); // 第一个inode为根目录
        save_bitmap();
    }
    void load_bitmap() {
        std::ifstream file(disk_path, std::ios::binary | std::ios::in);
        file.seekg(INODE_BITMAP_START * BLOCK_SIZE);
        file.read(reinterpret_cast<char *>(&bitmap), sizeof(bitmap));
        file.close();
    };
    void save_bitmap() {
        std::ofstream file(disk_path, std::ios::binary | std::ios::out | std::ios::in);
        file.seekp(INODE_BITMAP_START * BLOCK_SIZE);
        file.write(reinterpret_cast<const char *>(&bitmap), sizeof(bitmap));
        file.close();
    };

};
// 数据块位图
struct BlockBitmap{
    std::bitset<BLOCK_COUNT> bitmap;
    void init_bitmap()
    {
        bitmap.reset();
        //前600块已经被占用
        for (int i = 0; i < 600; i++){
            bitmap.set(i);
        } 
        save_bitmap();
    }
    void load_bitmap() {
        std::ifstream file(disk_path, std::ios::binary | std::ios::in);
        file.seekg(BLOCK_BITMAP_START * BLOCK_SIZE);
        file.read(reinterpret_cast<char *>(&bitmap), sizeof(bitmap));
        file.close();
    };
    void save_bitmap() {
        std::ofstream file(disk_path, std::ios::binary | std::ios::out | std::ios::in);
        file.seekp(BLOCK_BITMAP_START * BLOCK_SIZE);
        file.write(reinterpret_cast<const char *>(&bitmap), sizeof(bitmap));
        file.close();
    }
};

void init_disk(){
    std::ofstream file(disk_path, std::ios::binary | std::ios::out);
    // 初始化 写入 100MB 的0x00数据
    const int fileSize = FS_SIZE;
    const int bufferSize = 4096;
    char buffer[bufferSize] = {0};
    for (int i = 0; i < fileSize; i += bufferSize)
    {
        file.write(buffer, bufferSize);
    }
    file.close();
    // 初始化超级块，修改位图信息
    SuperBlock sb = {
        FS_SIZE,
        BLOCK_SIZE,
        INODE_COUNT,
        BLOCK_COUNT,
        FREE_INODES,
        FREE_BLOCKS,
        INODE_BITMAP_START,
        BLOCK_BITMAP_START,
        INODE_LIST_START,
        DATA_BLOCK_START};
    sb.writeToFile(disk_path, 0);

    // 初始化根目录等

    file.close();
}

int main()
{
    // inode_bitmap.load_bitmap();
    // std::cout << inode_bitmap.bitmap << std::endl;
    // printf("sizeof(Inode) = %d\n", sizeof(inode_bitmap));
}