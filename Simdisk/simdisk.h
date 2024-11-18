#pragma once
#include <bitset>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
const std::string disk_path = "../Disk/MyDisk.dat";

// 定义文件系统的一些参数, 通过设计得到
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
// 0-目录文件 1-普通文件 2-符号链接文件
#define DIR_TYPE 0
#define FILE_TYPE 1
#define LINK_TYPE 2
#define UNDEFINE_TYPE 3
// 类原型
struct InodeBitmap;
struct BlockBitmap;
struct SuperBlock;
struct Inode;
struct DirEntry;
struct DirBlock;
//输出相关
const std::string ERROR = "\033[31m";
const std::string SUCCESS = "\033[33m";
const std::string PATH = "\033[01;34m";
const std::string NORMAL = "\033[0m";
const std::string USER = "\033[01;32m";
/******************辅助函数*********************/

/**
 * @brief 将时间戳转为字符串
 * @param time 时间戳
 * @return 当前时区的时间字符串
 */
std::string format_time(uint32_t time);

/**
 * @brief 获取绝对路径
 * @param inode_id 目标文件夹的id
 * @return 绝对路径
 */
std::string get_absolute_path(uint32_t inode_id);

/**
 * @brief 确定某个路径是否是文件夹
 * 如果路径正确则将目标文件夹的id存储在purpose_id中，并返回True
 * 如果路径错误则输出错误并返回False，
 * @param path 需要被解析的路径，包括相对路径和绝对路径
 * @param purpose_id 目标文件夹的id
 * @return True 或者 False
 */
bool is_path_dir(const std::string &path, uint32_t &purpose_id);

/**
 * @brief 确定目录是否存在
 * 如果目录存在则将目标文件夹的id存储在purpose_id中，并返回True
 * 如果目录不存在则返回False
 * @param path 目录路径
 * @param purpose_id 目标文件夹的id
 * @return True 或者 False
 */
bool is_dir_exit(const std::string &path, uint32_t &purpose_id);

/**
 * @brief 确定当前目录下文件是否存在
 * @param name 文件名
 * @param cur_inode 当前目录的inode
 */
bool is_file_exit(const std::string &name, Inode cur_inode);
/**
 * @brief 确定目录名是否合法
 * 目录名不能包含/，且长度不能超过28，且不能为.和..
 * @param dir_name 目录名
 * @return True 或者 False
 */
bool is_valid_dir_name(const std::string &dir_name);

/**
 * @brief 获取文件的inode_id
 * @param file_name 文件名
 * @param dir_inode 当前目录的inode
 * @return 文件的inode_id
 */
uint32_t get_file_inode_id(const std::string &file_name, Inode &dir_inode);

/**
 * @brief 获取某个目录的绝对路径
 * @param inode_id 某个目录的id
 * @return 该目录的绝对路径
 */
std::string get_absolute_path(uint32_t inode_id);

/**
 * @brief 在当前目录下创建一级目录
 * @param dir_name 目录名
 * @param cur_inode 当前目录的inode
 * @return 下一级目录的inode_id
 */
uint32_t make_dir_help(const std::string &dir_name, Inode &cur_inode);

/**
 * @brief 读取文件内容
 * @param file_id 文件的inode_id
 * @return 文件内容
 */
std::string read_file(std::string file_path, std::string file_name);

/**
 * @brief 写入文件内容
 * @param file_path 文件的路径
 * @param file_name 文件名
 * @param content 文件内容
 * @return 是否写入成功
 */
bool write_file(std::string file_path, std::string file_name, std::string content);

/**
 * @brief 读取文件内容
 * @param file_path 文件的路径
 * @param file_name 文件名
 * @return 文件内容
 */
bool is_dir_empty(const uint32_t dir_inode_id);
/******************shell命令函数*********************/
// init 命令：磁盘格式化
void init_disk();
// dir 命令：显示当前目录的文件和目录
void show_directory(uint32_t inode_id);
// md 命令：创建目录
bool make_dir(const std::string dir_name, Inode cur_inode);
// newfile 命令：创建文件
bool make_file(const std::string file_name, uint32_t inode_id);
// del 命令：删除文件
bool del_file(const std::string file_name, Inode &cur_inode);
// rd 命令：删除目录
bool del_dir(const uint32_t the_purpose_dir_inode_id);
