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

#define INODE_SIZE 48

void init_disk();