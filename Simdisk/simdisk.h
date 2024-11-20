/**
 * @file simdisk.h
 * @brief 包含文件系统的一些参数、结构体定义、函数声明
 * @author Hu Yuzhi
 * @date 2024-11-20
 */

#pragma once
#include "encrypt.h"
#include <bitset>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

//------------------------------------------------------------------------------------------------
// 文件系统的一些参数
//------------------------------------------------------------------------------------------------

#define FS_SIZE 104857600
#define BLOCK_SIZE 1024
#define INODE_COUNT 12032
#define BLOCK_COUNT 102400
#define FREE_INODES 12032
#define FREE_BLOCKS 101800
#define INODE_BITMAP_START 14
#define BLOCK_BITMAP_START 1
#define INODE_LIST_START 16
#define DATA_BLOCK_START 600
// inode 相关
#define INODE_SIZE 48
// 0-目录文件 1-普通文件 2-符号链接文件 3-未定义
#define DIR_TYPE 0
#define FILE_TYPE 1
#define LINK_TYPE 2
#define UNDEFINE_TYPE 3

//------------------------------------------------------------------------------------------------
// 结构体声明
//------------------------------------------------------------------------------------------------
struct InodeBitmap;
struct BlockBitmap;
struct SuperBlock;
struct Inode;
struct DirEntry;
struct DirBlock;
struct IndexBlock;
struct User;

//------------------------------------------------------------------------------------------------
// 函数声明
//------------------------------------------------------------------------------------------------

std::string format_time(uint32_t time);
std::string get_absolute_path(uint32_t inode_id);
bool is_path_dir(const std::string &path, uint32_t &purpose_id, std::string &shell_output);
bool is_dir_exit(const std::string &path, uint32_t &purpose_id);
bool is_file_exit(const std::string &name, Inode cur_inode);
bool is_valid_dir_name(const std::string &dir_name);
uint32_t get_file_inode_id(const std::string &file_name, Inode &dir_inode);
uint32_t make_dir_help(const std::string &dir_name, Inode &cur_inode, User cur_user, uint32_t mode = 755);
std::string read_file(std::string file_path, std::string file_name);
bool write_file(std::string file_path, std::string file_name, std::string content);
bool is_dir_empty(const uint32_t dir_inode_id);
void init_disk();
std::string show_directory(uint32_t inode_id, bool show_recursion = false);
bool make_dir(const std::string dir_name, Inode cur_inode, User cur_user, uint32_t mode = 755);
bool make_file(const std::string file_name, uint32_t inode_id, std::string &_shell_output, User cur_user, uint32_t mode = 755);
bool del_file(const std::string file_name, Inode &cur_inode, std::string &shell_output);
bool del_dir(const uint32_t the_purpose_dir_inode_id, std::string &shell_output);
bool login(const std::string &user, const std::string &password, std::string &_shell_output, User &__user);
bool adduser(const std::string &user, const std::string &password, uint32_t uid, uint32_t gid);
std::string hash_pwd(const std::string &pwd);
bool is_able_to_write(const uint32_t inode_id, const User cur_user);
bool is_able_to_read(const uint32_t inode_id, const User cur_user);
bool is_able_to_execute(const uint32_t inode_id, const User cur_user);

//------------------------------------------------------------------------------------------------
// 全局变量
//------------------------------------------------------------------------------------------------

const std::string disk_path = "../Disk/MyDisk.dat";
extern InodeBitmap inode_bitmap;
extern BlockBitmap block_bitmap;
// 输出相关
const std::string __ERROR = "\033[31m";
const std::string __SUCCESS = "\033[36m";
const std::string __PATH = "\033[01;34m";
const std::string __NORMAL = "\033[0m";
const std::string __USER = "\033[01;32m";

//------------------------------------------------------------------------------------------------
// 结构体定义
//------------------------------------------------------------------------------------------------

/**
 * Inode位图
 */
struct InodeBitmap {
    std::bitset<INODE_COUNT> bitmap;
    InodeBitmap() { load_bitmap(); }

    /**
     * @brief 初始化inode位图
     */
    void init_bitmap() {
        bitmap.reset();
        save_bitmap();
    }

    /**
     * @brief 从文件中读取inode位图
     */
    void load_bitmap() {
        std::ifstream file(disk_path, std::ios::binary | std::ios::in);
        file.seekg(INODE_BITMAP_START * BLOCK_SIZE);
        file.read(reinterpret_cast<char *>(&bitmap), sizeof(bitmap));
        file.close();
    };

    /**
     * @brief 保存inode位图到文件
     */
    void save_bitmap() {
        std::ofstream file(disk_path, std::ios::binary | std::ios::out | std::ios::in);
        file.seekp(INODE_BITMAP_START * BLOCK_SIZE);
        file.write(reinterpret_cast<const char *>(&bitmap), sizeof(bitmap));
        file.close();
    };

    /**
     * @brief 获取一个空闲inode
     */
    uint32_t get_free_inode() {
        for (uint32_t i = 0; i < INODE_COUNT; i++) {
            if (!bitmap.test(i)) {
                bitmap.set(i);
                save_bitmap();
                // printf("get_free_inode %d", i);
                return i;
            }
        }
        return static_cast<uint32_t>(-1);
    }

    /**
     * @brief 释放一个inode, 使其变为空闲
     * @param inode_id inode编号
     */
    void free_inode(uint32_t inode_id) {
        bitmap.reset(inode_id);
        save_bitmap();
    }
};

/**
 * 数据块位图
 */
struct BlockBitmap {
    std::bitset<BLOCK_COUNT> bitmap;
    BlockBitmap() { load_bitmap(); }

    /**
     * @brief 初始化数据块位图
     */
    void init_bitmap() {
        bitmap.reset();
        // 前600块已经被占用
        for (int i = 0; i < 600; i++) {
            bitmap.set(i);
        }
        save_bitmap();
    }

    /**
     * @brief 从文件中读取数据块位图
     */
    void load_bitmap() {
        std::ifstream file(disk_path, std::ios::binary | std::ios::in);
        file.seekg(BLOCK_BITMAP_START * BLOCK_SIZE);
        file.read(reinterpret_cast<char *>(&bitmap), sizeof(bitmap));
        file.close();
    };

    /**
     * @brief 保存数据块位图到文件
     */
    void save_bitmap() {
        std::ofstream file(disk_path, std::ios::binary | std::ios::out | std::ios::in);
        file.seekp(BLOCK_BITMAP_START * BLOCK_SIZE);
        file.write(reinterpret_cast<const char *>(&bitmap), sizeof(bitmap));
        file.close();
    }

    /**
     * @brief 获取一个空闲数据块
     * @return 数据块号
     */
    uint32_t get_free_block() {
        for (uint32_t i = 600; i < BLOCK_COUNT; i++) {
            if (!bitmap.test(i)) {
                bitmap.set(i);
                save_bitmap();
                // printf("get_free_inode %d", i);
                return i;
            }
        }
        return static_cast<uint32_t>(-1);
    }

    /**
     * @brief 释放一个数据块, 使其变为空闲
     * @param block_id 数据块号
     */
    void free_block(uint32_t block_id) {
        bitmap.reset(block_id);
        save_bitmap();
    }
};

/**
 * 超级块结构体
 */
struct SuperBlock {
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
    uint32_t ctime;              // 创建时间
    uint32_t last_load_time;     // 最近加载时间

    /**
     * @brief 保存超级块到文件
     * @param filename 文件名，默认为磁盘中文件的位置
     * @param block_num 超级块所在的块号，默认为0
     */
    void save_super_block(const std::string &filename = disk_path, std::streampos block_num = 0) {
        std::ofstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
        if (!file) {
            std::cerr << "Error opening file for writing: " << filename << std::endl;
            return;
        }
        block_bitmap.load_bitmap();
        inode_bitmap.load_bitmap();
        free_blocks = BLOCK_COUNT - block_bitmap.bitmap.count();
        free_inodes = INODE_COUNT - inode_bitmap.bitmap.count();
        file.seekp(block_num * BLOCK_SIZE);
        file.write(reinterpret_cast<const char *>(this), sizeof(SuperBlock));
        file.close();
    }

    /**
     * @brief 从文件中读取超级块
     * @param filename 文件名，默认为磁盘中文件的位置
     * @param block_num 超级块所在的块号，默认为0
     * @return 读取到的超级块
     */
    static SuperBlock read_super_block(const std::string &filename = disk_path, std::streampos block_num = 0) {
        SuperBlock sb;
        std::ifstream ifs(filename, std::ios::binary);
        if (!ifs) {
            std::cerr << "Error opening file for reading: " << filename << std::endl;
            return sb;
        }
        ifs.seekg(block_num * BLOCK_SIZE);
        ifs.read(reinterpret_cast<char *>(&sb), sizeof(SuperBlock)); //??
        ifs.close();
        return sb;
    }

    /**
     * @brief 打印超级块信息
     */
    std::string print_super_block() const {
        std::ostringstream oss;
        oss << "文件系统大小: \t" << fs_size << " Bytes" << std::endl;
        oss << "总块数: \t" << block_count << std::endl;
        oss << "可用块数: \t" << free_blocks << std::endl;
        oss << "总inode数: \t" << inode_count << std::endl;
        oss << "可用inode数: \t" << free_inodes << std::endl;
        oss << "创建时间: \t" << format_time(ctime) << std::endl;
        oss << "最近加载时间: \t" << format_time(last_load_time) << std::endl;
        oss << "more..." << std::endl;
        return oss.str();
    }
};

/**
 * inode 结构体
 */
struct Inode {
    // 考虑对齐与填充，类型都设置为uint32_t
    uint32_t i_id;          // inode 编号, 表示为位置
    uint32_t i_size;        // 文件大小, 以字节为单位
    uint32_t i_blocks;      // 文件所占数据块数量
    uint32_t i_links_count; // 链接数
    uint32_t i_indirect;    // 一级间接块指针 0-102399
    uint32_t i_type;        // 文件类型 0-目录文件 1-普通文件 2-符号链接文件

    /*访问控制*/
    uint32_t i_mode; // 文件权限
    uint32_t i_uid;  // 用户 ID
    uint32_t i_gid;  // 组 ID
    /*时间戳*/
    uint32_t i_ctime; // 创建时间
    uint32_t i_mtime; // 修改时间
    uint32_t i_atime; // 访问时间

    /**
     * @brief 保存inode到文件
     */
    void save_inode() {
        std::ofstream file(disk_path, std::ios::binary | std::ios::out | std::ios::in);
        file.seekp(INODE_LIST_START * BLOCK_SIZE + i_id * INODE_SIZE);
        file.write(reinterpret_cast<const char *>(this), sizeof(Inode));
        file.close();
    }

    /**
     * @brief 从文件中读取inode
     * @param inode_id inode编号
     * @return 读取到的inode
     */
    static Inode read_inode(uint32_t inode_id) {
        Inode inode;
        std::ifstream file(disk_path, std::ios::binary | std::ios::in);
        file.seekg(INODE_LIST_START * BLOCK_SIZE + inode_id * INODE_SIZE);
        file.read(reinterpret_cast<char *>(&inode), sizeof(Inode));
        file.close();
        return inode;
    }
};

/**
 * 目录项结构体
 */
struct DirEntry {
    uint16_t inode_id; // inode < 12032
    uint16_t type;     // 文件类型 0-目录文件 1-普通文件 2-符号链接文件
    char name[28];     // 文件名

    /**
     * @brief 设置目录项
     * @param inode_id inode编号
     * @param type 文件类型
     * @param name 文件名
     */
    void set(uint16_t inode_id, uint16_t type, const char *name) {
        this->inode_id = inode_id;
        this->type = type;
        strcpy(this->name, name);
    }
};

/**
 * 目录数据块
 * 存储32个目录项 DirEntry
 */
struct DirBlock {
    DirEntry entries[32];

    /**
     * @brief 初始化目录块
     * @param parent_inode_id 父目录的inode_id
     * @param self_inode_id 当前目录的inode_id
     */
    void init_DirBlock(uint32_t parent_inode_id, uint32_t self_inode_id) {
        entries[0].set(self_inode_id, DIR_TYPE, ".");    // 当前目录
        entries[1].set(parent_inode_id, DIR_TYPE, ".."); // 父目录,如何得到父目录的inode_id？设置一个当前目录吗?
        for (int i = 2; i < 32; i++) {
            entries[i].set(UINT16_MAX, UNDEFINE_TYPE, "");
        }
    }

    /**
     * @brief 保存目录块到文件
     * @param block_id 目录块号
     */
    void save_dir_block(uint32_t block_id) {
        std::ofstream file(disk_path, std::ios::binary | std::ios::out | std::ios::in);
        file.seekp(block_id * BLOCK_SIZE);
        file.write(reinterpret_cast<const char *>(this), sizeof(DirBlock));
        file.close();
    }

    /**
     * @brief 从文件中读取目录块
     * @param block_id 目录块号
     * @return 读取到的目录块
     */
    static DirBlock read_dir_block(uint32_t block_id) {
        DirBlock db;
        std::ifstream file(disk_path, std::ios::binary | std::ios::in);
        file.seekg(block_id * BLOCK_SIZE);
        file.read(reinterpret_cast<char *>(&db), sizeof(DirBlock));
        file.close();
        return db;
    }
};

/**
 * 索引块
 * 一个索引块存储254个索引，每个索引指向一个数据块
 * 保留当前索引块的块号，以及下一个索引块的块号
 */
struct IndexBlock {
    uint32_t block_id;
    uint32_t next_index;
    uint32_t index[254];
    IndexBlock() {}
    IndexBlock(uint32_t id) {
        memset(index, UINT32_MAX, sizeof(index));
        next_index = UINT32_MAX;
        block_id = id;
        index[0] = block_bitmap.get_free_block();
    }

    /**
     * @brief 保存索引块到文件
     */
    void save_index_block() {
        std::ofstream file(disk_path, std::ios::binary | std::ios::out | std::ios::in);
        file.seekp(block_id * BLOCK_SIZE);
        file.write(reinterpret_cast<const char *>(this), sizeof(IndexBlock));
        file.close();
    }

    /**
     * @brief 从文件中读取索引块
     * @param id 索引块的块号
     * @return 读取到的索引块
     */
    static IndexBlock read_index_block(uint32_t id) {
        IndexBlock ib;
        std::ifstream file(disk_path, std::ios::binary | std::ios::in);
        file.seekg(id * BLOCK_SIZE);
        file.read(reinterpret_cast<char *>(&ib), sizeof(IndexBlock));
        file.close();
        return ib;
    }
};

struct User {
    std::string username;
    uint32_t uid;
    uint32_t gid;

    User() {}
    User(std::string _username, uint32_t _uid, uint32_t _gid) {
        username = _username;
        uid = _uid;
        gid = _gid;
    }
    void set(std::string _username, uint32_t _uid, uint32_t _gid) {
        username = _username;
        uid = _uid;
        gid = _gid;
    }
};

//------------------------------------------------------------------------------------------------
// 函数定义
//------------------------------------------------------------------------------------------------

/**
 * @brief 将时间戳转为字符串
 * @param time 时间戳
 * @return 当前时区的时间字符串
 */
std::string format_time(uint32_t raw_time) {
    time_t time = static_cast<time_t>(raw_time);
    struct tm *local_tm = localtime(&time);
    char local_buffer[80];
    strftime(local_buffer, 80, "%Y-%m-%d %H:%M:%S", local_tm);
    return std::string(local_buffer);
}

/**
 * @brief 创建或格式化磁盘
 * 初始化磁盘，写入100MB的0x00数据，初始化超级块，修改位图信息，创建根目录
 */
void init_disk() {
    std::ofstream file(disk_path, std::ios::binary | std::ios::out);
    // 初始化 写入 100MB 的0x00数据
    const int fileSize = FS_SIZE;
    const int bufferSize = 4096;
    char buffer[bufferSize] = {0};
    for (int i = 0; i < fileSize; i += bufferSize) {
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
        DATA_BLOCK_START,
        static_cast<uint32_t>(time(0)),
        static_cast<uint32_t>(time(0))};
    inode_bitmap.init_bitmap();
    block_bitmap.init_bitmap();
    // 创建根目录
    std::ofstream file1(disk_path, std::ios::binary | std::ios::out | std::ios::in);
    Inode root_inode = {
        inode_bitmap.get_free_inode(),  // inode 编号, 表示为位置
        sizeof(DirBlock),               // 文件大小, 初始化为两个目录项（.和..）
        2,                              // 文件所占数据块数量, 一个索引块和一个目录块
        1,                              // 链接数
        block_bitmap.get_free_block(),  // 一级间接块指针 600-102399
        DIR_TYPE,                       // 文件系统标志
        777,                            // 文件权限
        0,                              // 用户 ID
        0,                              // 组 ID
        static_cast<uint32_t>(time(0)), // 创建时间
        static_cast<uint32_t>(time(0)), // 修改时间
        static_cast<uint32_t>(time(0))  // 访问时间
    };
    root_inode.save_inode();
    IndexBlock root_ib(root_inode.i_indirect);
    root_ib.save_index_block();
    DirBlock root_db;
    root_db.init_DirBlock(root_inode.i_id, root_inode.i_id);
    root_db.save_dir_block(root_ib.index[0]);
    // 添加一个root用户
    adduser("root", "240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9", 0, 0);
    // 最后保存超级块
    sb.save_super_block(disk_path, 0);
}

/**
 * @brief 显示目录内容
 * @param inode_id 目录的inode_id
 * @return 目录的内容
 */
std::string show_directory(uint32_t inode_id, bool show_recursion) {
    std::vector<uint32_t> sons;
    std::ostringstream result;
    Inode inode = Inode::read_inode(inode_id);
    if (inode.i_type != DIR_TYPE) {
        return "";
    }
    std::string path = get_absolute_path(inode_id);
    result << __SUCCESS << "目录: " << path << __NORMAL << std::endl;
    IndexBlock index_block = IndexBlock::read_index_block(inode.i_indirect);
    result << std::left << std::setw(18) << "name" << std::setw(10) << "mode" << std::setw(10) << "size" << std::setw(10) << "last change" << std::endl;
    // std::cout << std::left << std::setw(10) << "name" << std::setw(10) << "mode" << std::setw(10) << "size" << std::setw(10) << "last change" << std::endl;
    for (uint32_t i = 0; i < 254; ++i) { // 访问索引块
        if (index_block.index[i] == UINT32_MAX) {
            break;
        }
        DirBlock dir_block = DirBlock::read_dir_block(index_block.index[i]);
        for (int j = 0; j < 32; ++j) {
            if (dir_block.entries[j].type == UNDEFINE_TYPE) {
                continue;
            }
            Inode temp_inode = Inode::read_inode(dir_block.entries[j].inode_id);
            std::string print_name = std::string(dir_block.entries[j].name);
            if (dir_block.entries[j].type == DIR_TYPE) {
                if (std::string(dir_block.entries[j].name) != "." && std::string(dir_block.entries[j].name) != "..") {
                    sons.push_back(dir_block.entries[j].inode_id);
                }
                print_name += "/";
            }
            result << std::left << std::setw(18) << print_name << std::setw(10) << temp_inode.i_mode << std::setw(10) << temp_inode.i_size << std::setw(10) << format_time(temp_inode.i_mtime) << std::endl;
            // std::cout << std::left << std::setw(10) << print_name << std::setw(10) << temp_inode.i_mode << std::setw(10) << temp_inode.i_size << std::setw(10) << format_time(temp_inode.i_mtime) << std::endl;
        }
    }
    std::string resultstr = result.str()+ "\n";
    if (show_recursion) {
        for (auto son_inode_id : sons) {
            resultstr += show_directory(son_inode_id, show_recursion);
        }
    }
    return resultstr;
}

/**
 * @brief 确定某个路径是否是文件夹
 * 如果路径正确则将目标文件夹的id存储在purpose_id中，并返回True
 * 如果路径错误则输出错误并返回False，
 * @param path 需要被解析的路径，包括相对路径和绝对路径
 * @param purpose_id 目标文件夹的id
 * @param _shell_output 输出信息
 * @return True 或者 False
 */
bool is_path_dir(const std::string &path, uint32_t &purpose_id, std::string &_shell_output) {
    // path 需要以 / 结尾，且不能包含 //
    if (path.find("//") != std::string::npos || path.back() != '/') {
        std::cerr << __ERROR << "输入路径格式错误，请重新输入" << __NORMAL << std::endl;
        _shell_output += __ERROR + "输入路径格式错误，请重新输入" + __NORMAL + "\n";
        return false;
    }

    std::vector<std::string> path_list;
    std::string temp;
    bool is_absolute = false; // 是否为绝对路径

    // 将路径按 '/' 分割成多个部分，存储在 path_list 中
    for (auto c : path) {
        if (c == '/' && !temp.empty()) {
            path_list.push_back(temp);
            temp.clear();
        } else if (c == '/' && temp.empty()) {
            is_absolute = true;
        } else {
            temp += c;
        }
    }

    uint32_t temp_inodeid;
    Inode temp_inode;

    if (is_absolute) {             // 绝对路径
        temp_inodeid = 0;          // 根目录 inode_id 为 0
    } else {                       // 相对路径
        temp_inodeid = purpose_id; // 从当前目录开始
    }
    for (const auto &p : path_list) {
        temp_inode = Inode::read_inode(temp_inodeid); // 开始目录的 inode
        IndexBlock temp_ib = IndexBlock::read_index_block(temp_inode.i_indirect);

        bool found = false;
        while (true) {
            for (uint32_t i = 0; i < 254; ++i) { // 访问索引块
                if (temp_ib.index[i] == UINT32_MAX) {
                    break;
                }

                // 读取目录块，遍历目录项，找到 p
                DirBlock dir_block = DirBlock::read_dir_block(temp_ib.index[i]);
                for (int j = 0; j < 32; ++j) {
                    if (dir_block.entries[j].type == UNDEFINE_TYPE) {
                        continue;
                    }
                    if (dir_block.entries[j].name == p && dir_block.entries[j].type == DIR_TYPE) {
                        temp_inodeid = dir_block.entries[j].inode_id;
                        found = true;
                        break;
                    }
                }
                if (found) {
                    break;
                }
            }
            if (found || temp_ib.next_index == UINT32_MAX) {
                break;
            }
            // 读取下一个 IndexBlock
            temp_ib = IndexBlock::read_index_block(temp_ib.next_index);
        }

        if (!found) {
            std::cerr << __ERROR << "目录不存在，请检查" << p << "是否正确" << __NORMAL << std::endl;
            _shell_output += __ERROR + "目录不存在，请检查" + p + "是否正确" + __NORMAL + "\n";
            return false;
        }
    }

    purpose_id = temp_inodeid;
    return true;
}

/**
 * @brief 确定目录是否存在
 * 如果目录存在则将目标文件夹的id存储在purpose_id中，并返回True
 * 如果目录不存在则返回False
 * @param path 目录路径
 * @param purpose_id 目标文件夹的id
 * @return True 或者 False
 */
bool is_dir_exit(const std::string &path, uint32_t &purpose_id) {
    std::vector<std::string> path_list;
    std::string temp;
    bool is_absolute = false; // 是否为绝对路径

    // 将路径按 '/' 分割成多个部分，存储在 path_list 中
    for (auto c : path) {
        if (c == '/' && !temp.empty()) {
            path_list.push_back(temp);
            temp.clear();
        } else if (c == '/' && temp.empty()) {
            is_absolute = true;
        } else {
            temp += c;
        }
    }

    uint32_t temp_inodeid;
    Inode temp_inode;

    if (is_absolute) {             // 绝对路径
        temp_inodeid = 0;          // 根目录 inode_id 为 0
    } else {                       // 相对路径
        temp_inodeid = purpose_id; // 从当前目录开始
    }
    for (const auto &p : path_list) {
        temp_inode = Inode::read_inode(temp_inodeid); // 开始目录的 inode
        IndexBlock temp_ib = IndexBlock::read_index_block(temp_inode.i_indirect);

        bool found = false;
        while (true) {
            for (uint32_t i = 0; i < 254; ++i) {
                if (temp_ib.index[i] == UINT32_MAX) {
                    break;
                }
                DirBlock dir_block = DirBlock::read_dir_block(temp_ib.index[i]);
                for (int j = 0; j < 32; ++j) {
                    if (dir_block.entries[j].type == UNDEFINE_TYPE) {
                        continue;
                    }
                    if (dir_block.entries[j].name == p) {
                        temp_inodeid = dir_block.entries[j].inode_id;
                        found = true;
                        break;
                    }
                }
                if (found) {
                    break;
                }
            }
            if (found || temp_ib.next_index == UINT32_MAX) {
                break;
            }
            temp_ib = IndexBlock::read_index_block(temp_ib.next_index);
        }
        if (!found) {
            return false;
        }
    }
    purpose_id = temp_inodeid;
    return true;
}

/**
 * @brief 确定当前目录下文件是否存在
 * @param name 文件名
 * @param cur_inode 当前目录的inode
 */
bool is_file_exit(const std::string &name, Inode cur_inode) {
    IndexBlock cur_ib = IndexBlock::read_index_block(cur_inode.i_indirect);

    bool found = false;
    while (true) {
        for (uint32_t i = 0; i < 254; ++i) {
            if (cur_ib.index[i] == UINT32_MAX) {
                break;
            }
            DirBlock dir_block = DirBlock::read_dir_block(cur_ib.index[i]);
            for (int j = 0; j < 32; ++j) {
                if (dir_block.entries[j].type == UNDEFINE_TYPE) {
                    continue;
                }
                if (dir_block.entries[j].name == name && dir_block.entries[j].type == FILE_TYPE) {
                    found = true;
                    break;
                }
            }
            if (found) {
                break;
            }
        }
        if (found || cur_ib.next_index == UINT32_MAX) {
            break;
        }
        cur_ib = IndexBlock::read_index_block(cur_ib.next_index);
    }
    if (!found) {
        return false;
    }
    return true;
}

/**
 * @brief 获取绝对路径
 * @param inode_id 目标文件夹的id
 * @return 绝对路径
 */
std::string get_absolute_path(uint32_t inode_id) {
    // 从 inode_id 开始，查找父目录，直到根目录
    // 可能有问题，待调式
    if (inode_id == 0) {
        return "/";
    }
    std::string path = "/";
    Inode inode = Inode::read_inode(inode_id);
    while (inode.i_id != 0) {
        IndexBlock index_block = IndexBlock::read_index_block(inode.i_indirect);
        DirBlock dir_block = DirBlock::read_dir_block(index_block.index[0]);
        Inode parent_inode;
        bool found = false;
        for (int i = 0; i < 32; i++) {
            // 注意写条件 char[28] 和 ""
            if (std::string(dir_block.entries[i].name) == "..") {
                parent_inode = Inode::read_inode(dir_block.entries[i].inode_id);
                IndexBlock parent_ib = IndexBlock::read_index_block(parent_inode.i_indirect);
                DirBlock parent_db = DirBlock::read_dir_block(parent_ib.index[0]);
                for (int j = 0; j < 32; j++) {
                    if (parent_db.entries[j].inode_id == inode.i_id) {
                        path = "/" + std::string(parent_db.entries[j].name) + path;
                        found = true;
                        break;
                    }
                }
                break;
            }
        }
        if (found) {
            inode = parent_inode;
        } else { // 未找到父目录，这是一个文件
            return "";
        }
    }
    return path;
}

/**
 * @brief 确定目录名是否合法
 * 目录名不能包含/，且长度不能超过28，且不能为.和..
 * @param dir_name 目录名
 * @return True 或者 False
 */
bool is_valid_dir_name(const std::string &dir_name) {
    // 目录不能包含 / ，长度不能超过28
    if (dir_name.find("/") != std::string::npos || dir_name.size() > 28) {
        return false;
    } else {
        return true;
    }
}

/**
 * @brief 在当前目录下创建一级目录
 * @param dir_name 目录名
 * @param cur_inode 当前目录的inode
 * @return 下一级目录的inode_id
 */
uint32_t make_dir_help(const std::string &dir_name, Inode &cur_inode, User cur_user, uint32_t mode) {
    IndexBlock cur_ib = IndexBlock::read_index_block(cur_inode.i_indirect);
    bool write_flag = false;
    Inode new_inode = {
        inode_bitmap.get_free_inode(),
        sizeof(DirBlock),
        2,
        1,
        block_bitmap.get_free_block(),
        DIR_TYPE,
        mode,
        cur_user.uid,
        cur_user.gid,
        static_cast<uint32_t>(time(0)),
        static_cast<uint32_t>(time(0)),
        static_cast<uint32_t>(time(0))};
    new_inode.save_inode();
    IndexBlock new_ib(new_inode.i_indirect);
    new_ib.save_index_block();
    DirBlock new_db;
    // 初始化目录项, 当前目录和父目录
    new_db.init_DirBlock(cur_inode.i_id, new_inode.i_id);
    new_db.save_dir_block(new_ib.index[0]);
    // 访问索引块的所有目录数据块，写入新目录dirname停止
    for (uint32_t i = 0; i < 254; i++) {
        if (cur_ib.index[i] == UINT32_MAX) {
            break;
        }
        DirBlock cur_db = DirBlock::read_dir_block(cur_ib.index[i]);
        for (int j = 0; j < 32; j++) {
            if (cur_db.entries[j].type == UNDEFINE_TYPE) {
                cur_db.entries[j].set(new_inode.i_id, DIR_TYPE, dir_name.c_str());
                write_flag = true;
                break;
            }
        }
        if (write_flag) {
            cur_db.save_dir_block(cur_ib.index[i]);
            break;
        }
    }
    // 更修父目录的修改时间
    cur_inode.i_mtime = static_cast<uint32_t>(time(0));
    cur_inode.save_inode();
    return new_inode.i_id;
}

/**
 * @brief 获取文件的inode_id
 * @param file_name 文件名
 * @param dir_inode 当前目录的inode
 * @return 文件的inode_id
 */
uint32_t get_file_inode_id(const std::string &file_name, Inode &dir_inode) {
    IndexBlock cur_ib = IndexBlock::read_index_block(dir_inode.i_indirect);
    bool found = false;
    while (true) {
        for (uint32_t i = 0; i < 254; i++) {
            if (cur_ib.index[i] == UINT32_MAX) {
                break;
            }
            DirBlock cur_db = DirBlock::read_dir_block(cur_ib.index[i]);
            for (int j = 0; j < 32; j++) {
                if (cur_db.entries[j].type == UNDEFINE_TYPE) {
                    continue;
                }
                if (cur_db.entries[j].name == file_name && cur_db.entries[j].type == FILE_TYPE) {
                    found = true;
                    return cur_db.entries[j].inode_id;
                }
            }
            if (found) {
                break;
            }
        }
        if (found || cur_ib.next_index == UINT32_MAX) {
            break;
        }
        cur_ib = IndexBlock::read_index_block(cur_ib.next_index);
    }
    return UINT32_MAX;
}

/**
 * @brief 读取文件内容
 * @param file_id 文件的inode_id
 * @return 文件内容
 */
std::string read_file(std::string file_path, std::string file_name) {
    uint32_t start_id = 0;
    if (!is_dir_exit(file_path, start_id)) {
        std::cout << __ERROR << "目标目录" << file_path << "不存在" << __NORMAL << std::endl;
        return "";
    }
    Inode dir_inode = Inode::read_inode(start_id);
    if (is_file_exit(file_name, dir_inode)) {
        // 获取并读取文件inode
        Inode file_inode = Inode::read_inode(get_file_inode_id(file_name, dir_inode));
        int file_size = file_inode.i_size;
        // 读取文件内容并转化为字符串
        std::string file_content;
        IndexBlock file_ib = IndexBlock::read_index_block(file_inode.i_indirect);
        int bytes_read = 0;
        for (uint32_t i = 0; i < 254 && bytes_read < file_size; i++) {
            if (file_ib.index[i] == UINT32_MAX) {
                break;
            }
            char buffer[BLOCK_SIZE];
            std::ifstream file(disk_path, std::ios::binary | std::ios::in);
            file.seekg(file_ib.index[i] * BLOCK_SIZE);
            int bytes_to_read = std::min(BLOCK_SIZE, file_size - bytes_read);
            file.read(buffer, bytes_to_read);
            file_content.append(buffer, bytes_to_read);
            bytes_read += bytes_to_read;
            file.close();
        }

        if (file_content.empty()) {
            return __SUCCESS+"it is empty";
        }
        return file_content;
    } else {
        std::cout << __ERROR << "目标文件" << file_name << "不存在" << __NORMAL << std::endl;
        return "";
    }
}

/**
 * @brief 写入文件内容
 * @param file_path 文件的路径
 * @param file_name 文件名
 * @param content 文件内容
 * @return 是否写入成功
 */
bool write_file(std::string file_path, std::string file_name, std::string content) {
    uint32_t start_id = 0;
    if (!is_dir_exit(file_path, start_id)) {
        std::cout << __ERROR << "目标目录" << file_path << "不存在" << __NORMAL << std::endl;
        return false;
    }
    Inode dir_inode = Inode::read_inode(start_id);
    // 向一个已经存在的文件后增加内容
    if (is_file_exit(file_name, dir_inode)) {
        Inode file_inode = Inode::read_inode(get_file_inode_id(file_name, dir_inode));
        IndexBlock file_ib = IndexBlock::read_index_block(file_inode.i_indirect);
        uint32_t block_id = UINT16_MAX;
        uint32_t file_size = file_inode.i_size;
        int need_num = (content.size() + file_size) / BLOCK_SIZE + 1;
        // 找到对应的块并写入
        for (int i = 0; i < need_num; i++) {
            if (i < 254) {
                if (file_ib.index[i] == UINT32_MAX) {
                    file_ib.index[i] = block_bitmap.get_free_block();
                    file_inode.i_blocks++;
                }
                block_id = file_ib.index[i];
            } else {
                if (file_ib.next_index == UINT32_MAX) {
                    file_ib.next_index = block_bitmap.get_free_block();
                    file_inode.i_blocks++;
                }
                file_ib = IndexBlock::read_index_block(file_ib.next_index);
                block_id = file_ib.index[i - 254];
            }
            std::ofstream file(disk_path, std::ios::binary | std::ios::out | std::ios::in);
            file.seekp(block_id * BLOCK_SIZE + (i == 0 ? file_size % BLOCK_SIZE : 0));
            file.write(content.c_str() + i * BLOCK_SIZE, std::min(BLOCK_SIZE, static_cast<int>(content.size() - i * BLOCK_SIZE)));
            file.close();
        }
        file_inode.i_size += content.size();
        file_inode.i_mtime = dir_inode.i_mtime = static_cast<uint32_t>(time(0));
        file_inode.save_inode();
        dir_inode.save_inode();
        file_ib.save_index_block();
        return true;
    } else {
        std::cout << __ERROR << "目标文件" << file_name << "不存在" << __NORMAL << std::endl;
        return false;
    }
}

/**
 * @brief 清空文件内容
 * @param file_path 文件的路径
 * @param file_name 文件名
 * @return 是否清空成功
 */
bool clear_file(std::string file_path, std::string file_name) {
    uint32_t start_id = 0;
    if (!is_dir_exit(file_path, start_id)) {
        std::cout << __ERROR << "目标目录" << file_path << "不存在" << __NORMAL << std::endl;
        return false;
    }
    Inode dir_inode = Inode::read_inode(start_id);
    if (is_file_exit(file_name, dir_inode)) {
        Inode file_inode = Inode::read_inode(get_file_inode_id(file_name, dir_inode));
        IndexBlock file_ib = IndexBlock::read_index_block(file_inode.i_indirect);
        for (uint32_t i = 1; i < 254; i++) { // 保留第一个块
            if (file_ib.index[i] == UINT32_MAX) {
                break;
            }
            block_bitmap.free_block(file_ib.index[i]);
            file_inode.i_blocks--;
        }
        file_inode.i_size = 0;
        file_inode.i_mtime = dir_inode.i_mtime = static_cast<uint32_t>(time(0));
        file_inode.save_inode();
        dir_inode.save_inode();
        return true;
    } else {
        std::cout << __ERROR << "目标文件" << file_name << "不存在" << __NORMAL << std::endl;
        return false;
    }
}

/**
 * @brief 创建目录
 * 输入一定是需要创建的目录（即当前不存在的目录）
 * @param dir_name 目录名
 * @param cur_inode 当前目录的inode
 * @return 是否创建成功
 */
bool make_dir(const std::string dir_name, Inode cur_inode, User cur_user, uint32_t mode) {
    std::vector<std::string> path_list;
    std::string temp;
    for (auto c : dir_name) {
        if (c == '/' && !temp.empty()) {
            path_list.push_back(temp);
            if (!is_valid_dir_name(temp)) {
                std::cout << __ERROR << "目录名" << temp << "不合法" << __NORMAL << std::endl;
                return false;
            }
            temp.clear();
        } else if (c != '/') {
            temp += c;
        }
    }
    std::string base_path = get_absolute_path(cur_inode.i_id);
    for (auto p : path_list) {
        base_path += p + "/";
        uint32_t cur_id = cur_inode.i_id;
        if (is_dir_exit(base_path, cur_id)) {
            cur_inode = Inode::read_inode(cur_id);
            continue;
        } else {
            cur_id = make_dir_help(p, cur_inode, cur_user, mode);
            cur_inode = Inode::read_inode(cur_id);
        }
    }
    return true;
}

/**
 * @brief 创建文件
 * @param file_name 文件名
 * @param inode_id 当前目录的inode
 * @return 是否创建成功
 */
bool make_file(const std::string file_name, uint32_t inode_id, std::string &_shell_output, User cur_user, uint32_t mode) {
    Inode parent_inode = Inode::read_inode(inode_id);
    if (!is_file_exit(file_name, parent_inode)) {
        IndexBlock cur_ib = IndexBlock::read_index_block(parent_inode.i_indirect);
        Inode new_inode = {
            inode_bitmap.get_free_inode(),
            0,
            2,
            1,
            block_bitmap.get_free_block(),
            FILE_TYPE,
            mode,
            cur_user.uid,
            cur_user.gid,
            static_cast<uint32_t>(time(0)),
            static_cast<uint32_t>(time(0)),
            static_cast<uint32_t>(time(0))};
        IndexBlock new_ib(new_inode.i_indirect);
        IndexBlock parent_ib = IndexBlock::read_index_block(parent_inode.i_indirect);
        bool write_flag = false;
        while (true) {
            for (uint32_t i = 0; i < 254; i++) {
                if (parent_ib.index[i] == UINT32_MAX) {
                    break;
                }
                DirBlock parent_db = DirBlock::read_dir_block(parent_ib.index[i]);
                for (int j = 0; j < 32; j++) {
                    if (parent_db.entries[j].type == UNDEFINE_TYPE) {
                        parent_db.entries[j].set(new_inode.i_id, FILE_TYPE, file_name.c_str());
                        write_flag = true;
                        break;
                    }
                }
                if (write_flag) {
                    parent_db.save_dir_block(parent_ib.index[i]);
                    break;
                }
            }
            if (write_flag) {
                break;
            }
            // 当前索引块已满，开辟新的索引块
            if (parent_ib.next_index == UINT32_MAX) {
                parent_ib.next_index = block_bitmap.get_free_block();
                parent_ib.save_index_block();
            }
            parent_ib = IndexBlock::read_index_block(parent_ib.next_index);
        };
        parent_inode.i_mtime = static_cast<uint32_t>(time(0));
        // save all
        new_inode.save_inode();
        parent_inode.save_inode();
        new_ib.save_index_block();
        return true;
    } else {
        std::cout << __ERROR << "文件" << file_name << "已存在" << __NORMAL << std::endl;
        _shell_output = __ERROR + "文件" + file_name + "已存在\n" + __NORMAL;
        return false;
    }
}

/**
 * @brief 删除文件
 * @param file_name 文件名
 * @param cur_inode 当前目录的inode
 * @param _shell_output 输出信息
 * @return 是否删除成功
 */
bool del_file(const std::string file_name, Inode &cur_inode, std::string &_shell_output) {
    IndexBlock cur_ib = IndexBlock::read_index_block(cur_inode.i_indirect);
    bool found = false;
    uint32_t file_inode_id;
    // 删除目录项
    while (true) {
        for (uint32_t i = 0; i < 254; i++) {
            if (cur_ib.index[i] == UINT32_MAX) {
                break;
            }
            DirBlock cur_db = DirBlock::read_dir_block(cur_ib.index[i]);
            for (int j = 0; j < 32; j++) {
                if (cur_db.entries[j].type == UNDEFINE_TYPE) {
                    continue;
                }
                if (std::string(cur_db.entries[j].name) == file_name && cur_db.entries[j].type == FILE_TYPE) {
                    found = true;
                    file_inode_id = cur_db.entries[j].inode_id;
                    cur_db.entries[j].type = UNDEFINE_TYPE;
                    cur_db.entries[j].inode_id = UINT16_MAX;
                    cur_db.save_dir_block(cur_ib.index[i]);
                    break;
                }
            }
            if (found) {
                break;
            }
        }
        if (found || cur_ib.next_index == UINT32_MAX) {
            break;
        }
        cur_ib = IndexBlock::read_index_block(cur_ib.next_index);
    }
    if (!found) {
        std::cout << __ERROR << "文件" << file_name << "不存在" << __NORMAL << std::endl;
        _shell_output = __ERROR + "文件" + file_name + __NORMAL + "不存在";
        return false;
    }
    Inode file_inode = Inode::read_inode(file_inode_id);
    uint32_t file_ib_id = file_inode.i_indirect;
    IndexBlock file_ib = IndexBlock::read_index_block(file_ib_id);
    for (uint32_t i = 0; i < 254; i++) {
        if (file_ib.index[i] == UINT32_MAX) {
            break;
        }
        block_bitmap.free_block(file_ib.index[i]);
    }
    block_bitmap.free_block(file_ib_id);
    inode_bitmap.free_inode(file_inode_id);
    return true;
}

/**
 * @brief 读取文件内容
 * @param file_path 文件的路径
 * @param file_name 文件名
 * @return 文件内容
 */
bool is_dir_empty(const uint32_t dir_inode_id) {
    Inode dir_inode = Inode::read_inode(dir_inode_id);
    IndexBlock cur_ib = IndexBlock::read_index_block(dir_inode.i_indirect);
    for (uint32_t i = 0; i < 254; i++) {
        if (cur_ib.index[i] == UINT32_MAX) {
            break;
        }
        DirBlock cur_db = DirBlock::read_dir_block(cur_ib.index[i]);
        for (int j = 2; j < 32; j++) {
            if (cur_db.entries[j].type == UNDEFINE_TYPE) {
                continue;
            }
            if (cur_db.entries[j].type != UNDEFINE_TYPE) {
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief 删除目录
 * @param dir_inode_id 目录inode_id
 * @param _shell_output 输出信息
 * @return 是否删除成功
 */
bool del_dir(const uint32_t dir_inode_id, std::string &_shell_output) {
    uint32_t parent_inode_id = 0;
    Inode dir_inode = Inode::read_inode(dir_inode_id);
    IndexBlock cur_ib = IndexBlock::read_index_block(dir_inode.i_indirect);
    while (true) {
        for (uint32_t i = 0; i < 254; i++) {
            if (cur_ib.index[i] == UINT32_MAX) {
                break;
            }
            DirBlock cur_db = DirBlock::read_dir_block(cur_ib.index[i]);
            for (int j = 0; j < 32; j++) {
                // 跳过当前目录和父目录，递归后删除
                if (cur_db.entries[j].type == UNDEFINE_TYPE || std::string(cur_db.entries[j].name) == "." || std::string(cur_db.entries[j].name) == "..") {
                    if (std::string(cur_db.entries[j].name) == "..") {
                        parent_inode_id = cur_db.entries[j].inode_id;
                    }
                    continue;
                }
                if (cur_db.entries[j].type == DIR_TYPE) {
                    if (!del_dir(cur_db.entries[j].inode_id, _shell_output)) {
                        return false;
                    }
                } else if (cur_db.entries[j].type == FILE_TYPE) {
                    if (!del_file(cur_db.entries[j].name, dir_inode, _shell_output)) {
                        return false;
                    }
                }
            }
            block_bitmap.free_block(cur_ib.index[i]);
        }
        if (cur_ib.next_index == UINT32_MAX) {
            // 释放最后一个索引块
            block_bitmap.free_block(cur_ib.block_id);
            break;
        }
        // 释放当前索引块，更新下一个索引块
        block_bitmap.free_block(cur_ib.block_id);
        cur_ib = IndexBlock::read_index_block(cur_ib.next_index);
    }
    inode_bitmap.free_inode(dir_inode_id);
    Inode parent_inode = Inode::read_inode(parent_inode_id);
    IndexBlock parent_ib = IndexBlock::read_index_block(parent_inode.i_indirect);
    bool found = false;
    while (true) {
        for (uint32_t i = 0; i < 254; i++) {
            if (parent_ib.index[i] == UINT32_MAX) {
                break;
            }
            DirBlock parent_db = DirBlock::read_dir_block(parent_ib.index[i]);
            for (int j = 0; j < 32; j++) {
                if (parent_db.entries[j].type == UNDEFINE_TYPE) {
                    continue;
                }
                if (parent_db.entries[j].inode_id == dir_inode_id) {
                    parent_db.entries[j].type = UNDEFINE_TYPE;
                    parent_db.entries[j].inode_id = UINT16_MAX;
                    parent_db.entries[j].name[0] = '\0';
                    parent_db.save_dir_block(parent_ib.index[i]);
                    found = true;
                    break;
                }
            }
            if (found) {
                break;
            }
        }
        if (found || parent_ib.next_index == UINT32_MAX) {
            break;
        }
        parent_ib = IndexBlock::read_index_block(parent_ib.next_index);
    }
    return true;
}

/**
 * @brief 登录
 */
bool login(const std::string &user, const std::string &password, std::string &_shell_output, User &__user) {
    // 创建一个账号密码map
    std::map<std::string, std::string> user_pwd_pair;
    std::map<std::string, std::pair<uint32_t, uint32_t>> user_info; // 用于存储用户名对应的uid和gid
    std::string all_info = read_file("/etc/", "passwd");
    std::istringstream iss(all_info);
    std::string line;

    // 解析文件内容，填充user_pwd_pair和user_info
    while (std::getline(iss, line)) {
        std::istringstream line_stream(line);
        std::string username, hashed_password, uid_str, gid_str;

        if (std::getline(line_stream, username, ':') &&
            std::getline(line_stream, hashed_password, ':') &&
            std::getline(line_stream, uid_str, ':') &&
            std::getline(line_stream, gid_str, ':')) {
            uint32_t uid = std::stoul(uid_str);
            uint32_t gid = std::stoul(gid_str);
            user_pwd_pair[username] = hashed_password;
            user_info[username] = std::make_pair(uid, gid);
        }
    }
    // 检查用户是否存在
    auto it = user_pwd_pair.find(user);
    if (it == user_pwd_pair.end()) {
        _shell_output = __ERROR + "用户不存在，请向管理员申请" + __NORMAL + "\n";
        return false;
    }
    // 检查输入的用户名和密码
    if (it != user_pwd_pair.end()) {
        std::string hashed_input_password = hash_pwd(password);
        if (hashed_input_password == it->second) {
            // _shell_output = __SUCCESS + "登录成功"+ __NORMAL+"\n";
            __user.set(user, user_info[user].first, user_info[user].second);
            return true;
        }
    }

    _shell_output = __ERROR + "密码错误，请重新输入" + __NORMAL + "\n";
    return false;
}

/**
 * @brief 加密密码
 */
std::string hash_pwd(const std::string &pwd) {
    SHA256 sha256;
    sha256.update(pwd);
    return sha256.final();
}

bool adduser(const std::string &user, const std::string &password, uint32_t uid, uint32_t gid) {
    // 定义一个加密函数
    std::string pwd;
    if (password.length() != 64) {
        pwd = hash_pwd(password);
    } else {
        pwd = password;
    }
    std::string file_path = "/etc/";
    std::string file_name = "passwd";
    uint32_t start_id = 0;
    std::string shell_output;
    Inode root_inode = Inode::read_inode(0);
    User cur_user("root", 0, 0);
    if (is_dir_exit(file_path, start_id)) { // 目录存在
        make_file(file_name, start_id, shell_output, cur_user,710);
    } else { // 目录不存在
        if (make_dir(file_path, root_inode, cur_user)) {
            is_dir_exit(file_path, start_id); // 找到目录
            make_file(file_name, start_id, shell_output, cur_user,710);
        }
    }
    std::string content = user + ":" + pwd + ":" + std::to_string(uid) + ":" + std::to_string(gid) + "\n";
    return write_file(file_path, file_name, content);
}

/**
 * @brief 判断用户是否有权限对文件进行写
 * @param inode_id 文件或目录的inode_id
 * @param cur_user 当前用户
 * @return 是否有权限
 */
bool is_able_to_write(const uint32_t inode_id, const User cur_user) {
    Inode inode = Inode::read_inode(inode_id);
    uint32_t mode = inode.i_mode;
    if (inode.i_uid == cur_user.uid) {
        return (mode / 100 & 2) != 0;// 检查用户写权限
    } else if (inode.i_gid == cur_user.gid) {
        return ((mode / 10) % 10 & 2) != 0;// 检查组写权限
    } else {
        return (mode % 10 & 2) != 0;// 检查其他写权限
    }
}

/**
 * @brief 判断用户是否有权限对文件进行读
 * @param inode_id 文件或目录的inode_id
 * @param cur_user 当前用户
 * @return 是否有权限
 */
bool is_able_to_read(const uint32_t inode_id, const User cur_user) {
    Inode inode = Inode::read_inode(inode_id);
    uint32_t mode = inode.i_mode;
    if (inode.i_uid == cur_user.uid) {
        return (mode / 100 & 4) != 0;// 检查用户读权限
    } else if (inode.i_gid == cur_user.gid) {
        return ((mode / 10) % 10 & 4) != 0;// 检查组读权限
    } else {
        return (mode % 10 & 4) != 0;// 检查其他读权限
    }
}


/**
 * @brief 判断用户是否有权限对文件进行执行
 * @param inode_id 文件或目录的inode_id
 * @param cur_user 当前用户
 * @return 是否有权限
 */
bool is_able_to_execute(const uint32_t inode_id, const User cur_user) {
    Inode inode = Inode::read_inode(inode_id);
    uint32_t mode = inode.i_mode;
    if (inode.i_uid == cur_user.uid) {
        return (mode/100 & 1) != 0; // 检查用户执行权限
    } else if (inode.i_gid == cur_user.gid) {
        return ((mode/10)% 10 & 1) != 0; // 检查组执行权限
    } else {
        return (mode%10 & 1) != 0; // 检查其他执行权限
    }
}
