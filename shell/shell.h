/**
 * @file shell.h
 * @brief Shell的一些常量和以及用到的类与函数
 * @author Hu Yuzhi
 * @date 2024-11-20
 */

#pragma once

#include <algorithm>
#include <conio.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <mutex>
#include <sstream>
#include <vector>
#include <map>
#include <windows.h>
#include <fstream>
#include <iomanip>
#include "share_memory.h"

//------------------------------------------------------------------------------------------------
// 全局变量
//------------------------------------------------------------------------------------------------

const std::string disk_path = "../Disk/MyDisk.dat";
const std::string welcome1 =
    "    ____  __  __        _____            __                        \n"
    "   / __/ /_/ / /__     / ___/__  _______/ /____  ____ ___          \n"
    "  / /_  / / / / _ \\    \\__ \\/ / / / ___/ __/ _ \\/ __ `__ \\    \n"
    " / __/ / / / /  __/   ___/ / /_/ (__  / /_/  __/ / / / / /         \n"
    "/_/   /_/ /_/\\___/   /____/\\__, /____/\\__/\\___/_/ /_/ /_/      \n"
    "                          /____/                                 \n"
    "                                           --Provided by Hu Yuzhi  \n";

std::string welcome2 =
    "      ___                   ___  ___                   ___      ___         ___      ___         ___         ___               ___                     \n"
    "     /\\  \\        ___      /\\__\\/\\  \\                 /\\  \\    |\\__\\       /\\  \\    /\\  \\       /\\  \\       /\\__\\                    \n"
    "    /::\\  \\      /\\  \\    /:/  /::\\  \\               /::\\  \\   |:|  |     /::\\  \\   \\:\\  \\     /::\\  \\     /::|  |                      \n"
    "   /:/\\:\\  \\     \\:\\  \\  /:/  /:/\\:\\  \\             /:/\\ \\  \\  |:|  |    /:/\\ \\  \\   \\:\\  \\   /:/\\:\\  \\   /:|:|  |                \n"
    "  /::\\~\\:\\  \\    /::\\__\\/:/  /::\\~\\:\\  \\           _\\:\\~\\ \\  \\ |:|__|__ _\\:\\~\\ \\  \\  /::\\  \\ /::\\~\\:\\  \\ /:/|:|__|__         \n"
    " /:/\\:\\ \\:\\__\\__/:/\\/__/:/__/:/\\:\\ \\:\\__\\         /\\ \\:\\ \\ \\__\\/::::\\__/\\ \\:\\ \\ \\__\\/:/\\:\\__/:/\\:\\ \\:\\__/:/ |::::\\__\\  \n"
    " \\/__\\:\\ \\/__/\\/:/  /  \\:\\  \\:\\~\\:\\ \\/__/         \\:\\ \\:\\ \\/__/:/~~/~  \\:\\ \\:\\ \\/__/:/  \\/__\\:\\~\\:\\ \\/__\\/__/~~/:/  /     \n"
    "      \\:\\__\\ \\::/__/    \\:\\  \\:\\ \\:\\__\\            \\:\\ \\:\\__\\/:/  /     \\:\\ \\:\\__\\/:/  /     \\:\\ \\:\\__\\       /:/  /         \n"
    "       \\/__/  \\:\\__\\     \\:\\  \\:\\ \\/__/             \\:\\/:/  /\\/__/       \\:\\/:/  /\\/__/       \\:\\ \\/__/      /:/  /                  \n"
    "               \\/__/      \\:\\__\\:\\__\\                \\::/  /              \\::/  /              \\:\\__\\       /:/  /                          \n"
    "                           \\/__/\\/__/                 \\/__/                \\/__/                \\/__/       \\/__/                                \n"
    "                                                                                                                               --Provided by Hu Yuzhi  \n";

// 便于阅读的颜色
const std::string __ERROR = "\033[31m";
const std::string __SUCCESS = "\033[36m";
const std::string __PATH = "\033[01;34m";
const std::string __NORMAL = "\033[0m";
const std::string __USER = "\033[01;32m";

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
#define MAX_USER 10


//------------------------------------------------------------------------------------------------
// 类声明
//------------------------------------------------------------------------------------------------

struct Inode;
struct DirEntry;
struct DirBlock;
struct IndexBlock;
struct InodeBitmap;
struct BlockBitmap;
struct User;

//------------------------------------------------------------------------------------------------
// 类定义
//------------------------------------------------------------------------------------------------

/**
 * 用户信息
 */
struct User {
    std::string username;
    uint32_t uid;
    uint32_t gid;

    User(){}
    User(std::string _username, uint32_t _uid, uint32_t _gid) {
        username = _username;
        uid = _uid;
        gid = _gid;
    }
    void set(std::string _username,uint32_t _uid, uint32_t _gid) {
        username = _username;
        uid = _uid;
        gid = _gid;
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

uint32_t get_file_inode_id(const std::string &file_name, Inode &dir_inode);
std::string get_absolute_path(uint32_t inode_id) ;
bool is_dir_exit(const std::string &path, uint32_t &purpose_id);

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