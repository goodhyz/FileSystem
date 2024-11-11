#include "simdisk.h"
// 定义超级块结构体, 作为第一个块
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

    // 成员函数：将超级块写入文件
    void save_super_block(const std::string &filename = disk_path, std::streampos block_num = 0) const {
        std::ofstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
        if (!file) {
            std::cerr << "Error opening file for writing: " << filename << std::endl;
            return;
        }
        file.seekp(block_num * BLOCK_SIZE);
        file.write(reinterpret_cast<const char *>(this), sizeof(SuperBlock));
        file.close();
    }

    // 成员函数：从文件读取超级块
    static SuperBlock read_super_block(const std::string &filename = disk_path, std::streampos block_num = 0) {
        SuperBlock sb;
        std::ifstream ifs(filename, std::ios::binary);
        if (!ifs) {
            std::cerr << "Error opening file for reading: " << filename << std::endl;
            return sb;
        }
        ifs.seekg(block_num * BLOCK_SIZE);
        ifs.read(reinterpret_cast<char *>(&sb), sizeof(SuperBlock));
        ifs.close();
        return sb;
    }

    // 成员函数：返回文件系统信息
    // 统计总目录数和总文件数
    void print_super_block() const {
        std::cout << "文件系统大小: " << fs_size << "Bytes" << std::endl;
        std::cout << "总块数: " << block_size << "Bytes" << std::endl;
        std::cout << "可用块数: " << free_blocks << std::endl;
        std::cout << "总inode数: " << inode_count << std::endl;
        std::cout << "可用inode数: " << free_inodes << std::endl;
        std::cout << "more..."<< std::endl;
    }
};

// inode位图
struct InodeBitmap {
    std::bitset<INODE_COUNT> bitmap;
    void init_bitmap() {
        bitmap.reset();
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
};
// 数据块位图
struct BlockBitmap {
    std::bitset<BLOCK_COUNT> bitmap;
    void init_bitmap() {
        bitmap.reset();
        // 前600块已经被占用
        for (int i = 0; i < 600; i++) {
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
};

// 全局变量
InodeBitmap inode_bitmap;
BlockBitmap block_bitmap;

// 定义 inode 结构体
// inode 结构体大小为 48 字节，分配564块做inode表，2块做inode位图
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

    void save_inode() {
        std::ofstream file(disk_path, std::ios::binary | std::ios::out | std::ios::in);
        file.seekp(INODE_LIST_START * BLOCK_SIZE + i_id * INODE_SIZE);
        file.write(reinterpret_cast<const char *>(this), sizeof(Inode));
        file.close();
    }
    static Inode read_inode(uint32_t inode_id) {
        Inode inode;
        std::ifstream file(disk_path, std::ios::binary | std::ios::in);
        file.seekg(INODE_LIST_START * BLOCK_SIZE + inode_id * INODE_SIZE);
        file.read(reinterpret_cast<char *>(&inode), sizeof(Inode));
        file.close();
        return inode;
    }
};

// 定义目录项结构体, 32字节
struct DirEntry {
    uint16_t inode_id; // inode < 12032
    uint16_t type;     // 文件类型 0-目录文件 1-普通文件 2-符号链接文件
    char name[28];     // 文件名
    void set(uint16_t inode_id, uint16_t type, const char *name) {
        this->inode_id = inode_id;
        this->type = type;
        strcpy(this->name, name);
    }
};

// 目录数据块
struct DirBlock {
    DirEntry entries[32];
    void init_DirBlock(uint32_t parent_inode_id, uint32_t self_inode_id) {
        entries[0].set(self_inode_id, DIR_TYPE, ".");    // 当前目录
        entries[1].set(parent_inode_id, DIR_TYPE, ".."); // 父目录,如何得到父目录的inode_id？设置一个当前目录吗?
        for (int i = 2; i < 32; i++) {
            entries[i].set(UINT16_MAX, UNDEFINE_TYPE, "");
        }
    }
    void save_dir_block(uint32_t block_id) {
        std::ofstream file(disk_path, std::ios::binary | std::ios::out | std::ios::in);
        file.seekp(block_id * BLOCK_SIZE);
        file.write(reinterpret_cast<const char *>(this), sizeof(DirBlock));
        file.close();
    }
    static DirBlock read_dir_block(uint32_t block_id) {
        DirBlock db;
        std::ifstream file(disk_path, std::ios::binary | std::ios::in);
        file.seekg(block_id * BLOCK_SIZE);
        file.read(reinterpret_cast<char *>(&db), sizeof(DirBlock));
        file.close();
        return db;
    }
};

// 索引数据块
// 保留256个索引，设置最后一个作为更高一级索引
struct IndexBlock {
    uint32_t block_id;
    uint32_t next_index;
    uint32_t index[254];
    IndexBlock(uint32_t id) {
        memset(index, UINT32_MAX, sizeof(index));
        next_index = UINT32_MAX;
        block_id = id;
        index[0] = block_bitmap.get_free_block();
    }
    void save_index_block() {
        std::ofstream file(disk_path, std::ios::binary | std::ios::out | std::ios::in);
        file.seekp(block_id * BLOCK_SIZE);
        file.write(reinterpret_cast<const char *>(this), sizeof(IndexBlock));
        file.close();
    }
    static IndexBlock read_index_block(uint32_t id) {
        IndexBlock ib(id);
        std::ifstream file(disk_path, std::ios::binary | std::ios::in);
        file.seekg(id * BLOCK_SIZE);
        file.read(reinterpret_cast<char *>(&ib), sizeof(IndexBlock));
        file.close();
        return ib;
    }
};

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
        DATA_BLOCK_START};
    sb.save_super_block(disk_path, 0);
    inode_bitmap.init_bitmap();
    block_bitmap.init_bitmap();
    // 创建根目录
    std::ofstream file1(disk_path, std::ios::binary | std::ios::out | std::ios::in);
    Inode root_inode = {
        inode_bitmap.get_free_inode(),  // inode 编号, 表示为位置
        2 * sizeof(DirEntry),           // 文件大小, 初始化为两个目录项（.和..）
        2,                              // 文件所占数据块数量, 一个索引块和一个目录块
        1,                              // 链接数
        block_bitmap.get_free_block(),  // 一级间接块指针 600-102399
        DIR_TYPE,                       // 文件系统标志
        755,                            // 文件权限
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
    // 创建 保留索引数据块和目录数据块
    // 1.根据i_indirect创建索引块
    // 2.根据索引块创建目录数据块
}

std::string format_time(uint32_t raw_time) {
    time_t time = static_cast<time_t>(raw_time);
    struct tm *local_tm = localtime(&time);
    char local_buffer[80];
    strftime(local_buffer, 80, "%Y-%m-%d %H:%M:%S", local_tm);
    return std::string(local_buffer);
}

int main() {
    // init_disk();
    using namespace std;
    Inode root_inode = Inode::read_inode(0);
    cout << "root_inode.i_id: " << root_inode.i_id << endl;
    IndexBlock root_ib = IndexBlock::read_index_block(root_inode.i_indirect);
    cout << "root_ib.block_id: " << root_ib.block_id << endl;
    DirBlock root_db = DirBlock::read_dir_block(root_ib.index[0]);
    cout << "root_db.entries[0].inode_id: " << root_db.entries[0].inode_id << endl;
    cout << "root_db.entries[0].type: " << root_db.entries[0].type << endl;
    cout << "root_db.entries[0].name: " << root_db.entries[0].name << endl;
    cout << "root_db.entries[1].inode_id: " << root_db.entries[1].inode_id << endl;
    cout << "root_db.entries[1].type: " << root_db.entries[1].type << endl;
    cout << "root_db.entries[1].name: " << root_db.entries[1].name << endl;
}