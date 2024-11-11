const std::string disk_path = "../Disk/MyDisk.dat";

// 定义文件系统的一些参数, 通过设计得到
#define FS_SIZE 104857600
#define BLOCK_SIZE 1024
#define INODE_COUNT 12032
#define BLOCK_COUNT 102400
#define FREE_INODES 12032
#define FREE_BLOCKS 102400
#define INODE_BITMAP_START 14
#define BLOCK_BITMAP_START 1
#define INODE_LIST_START 16
#define DATA_BLOCK_START 600
// inode 相关
#define INODE_SIZE 48
//0-目录文件 1-普通文件 2-符号链接文件
#define DIR_TYPE 0
#define FILE_TYPE 1
#define LINK_TYPE 2

// 类原型
struct InodeBitmap;
struct BlockBitmap;
struct SuperBlock;
struct Inode;
struct DirEntry;
struct DirBlock;

//初始化磁盘
void init_disk();
//将uint32_t类型的时间转换为时间字符串
std::string format_time(uint32_t time);