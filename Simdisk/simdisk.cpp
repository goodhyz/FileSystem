#include <iostream>
#include <fstream>
#include <cstdint>
#include <ctime>
#include <string>

const std::string disk_path = "../Disk/MyDisk.dat";

// 定义超级块结构体, 作为第一个块
struct SuperBlock
{
    uint32_t fs_size;            // 文件系统大小 102400 blocks * 1 KB
    uint32_t block_size;         // 块大小（以字节为单位） 1 KB
    uint32_t inode_count;        // inode 总数
    uint32_t block_count;        // 数据块总数
    uint32_t free_inodes;        // 空闲 inode 数量
    uint32_t free_blocks;        // 空闲数据块数量
    uint32_t inode_bitmap_start; // inode 位图的起始位置
    uint32_t block_bitmap_start; // 数据块位图的起始位置
    uint32_t inode_list_start;   // inode 列表的起 始位置
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
        file.seekp(block_num * 1024);
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
        ifs.seekg(block_num * 1024);
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

void init_disk()
{
    std::ofstream file(disk_path, std::ios::binary | std::ios::out);
    if (!file)
    {
        std::cerr << "Error opening file for writing: " << disk_path << std::endl;
        return;
    }
    // 写入 100MB 的0x00数据
    const int fileSize = 102400 * 1024;
    const int bufferSize = 4096;
    char buffer[bufferSize] = {0};
    for (int i = 0; i < fileSize; i += bufferSize)
    {
        file.write(buffer, bufferSize);
    }

    file.close();
}

int main()
{
    init_disk();
    // // 将超级块写入文件
    // sb.writeToFile();

    // // 从文件读取超级块
    // SuperBlock sb_read = SuperBlock::readFromFile();
}