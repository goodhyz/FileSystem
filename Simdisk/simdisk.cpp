#include "simdisk.h"

// inode位图
struct InodeBitmap {
    std::bitset<INODE_COUNT> bitmap;
    InodeBitmap() { load_bitmap(); }
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
    void free_inode(uint32_t inode_id) {
        bitmap.reset(inode_id);
        save_bitmap();
    }
};
// 数据块位图
struct BlockBitmap {
    std::bitset<BLOCK_COUNT> bitmap;
    BlockBitmap() { load_bitmap(); }
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
    void free_block(uint32_t block_id) {
        bitmap.reset(block_id);
        save_bitmap();
    }
};

// 全局变量
InodeBitmap inode_bitmap;
BlockBitmap block_bitmap;

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
    uint32_t ctime;              // 创建时间
    uint32_t last_load_time;     // 最近加载时间

    // 成员函数：将超级块写入文件
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

    // 成员函数：从文件读取超级块
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

    // 成员函数：返回文件系统信息
    // 统计总目录数和总文件数
    void print_super_block() const {
        std::cout << "文件系统大小: \t" << fs_size << " Bytes" << std::endl;
        std::cout << "总块数: \t" << block_count << std::endl;
        std::cout << "可用块数: \t" << free_blocks << std::endl;
        std::cout << "总inode数: \t" << inode_count << std::endl;
        std::cout << "可用inode数: \t" << free_inodes << std::endl;
        std::cout << "创建时间: \t" << format_time(ctime) << std::endl;
        std::cout << "最近加载时间: \t" << format_time(last_load_time) << std::endl;
        std::cout << "more..." << std::endl;
    }
};
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
    IndexBlock() {}
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
        IndexBlock ib;
        std::ifstream file(disk_path, std::ios::binary | std::ios::in);
        file.seekg(id * BLOCK_SIZE);
        file.read(reinterpret_cast<char *>(&ib), sizeof(IndexBlock));
        file.close();
        return ib;
    }
};

std::string format_time(uint32_t raw_time) {
    time_t time = static_cast<time_t>(raw_time);
    struct tm *local_tm = localtime(&time);
    char local_buffer[80];
    strftime(local_buffer, 80, "%Y-%m-%d %H:%M:%S", local_tm);
    return std::string(local_buffer);
}

// init 命令：磁盘格式化
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
    // 最后保存超级块
    sb.save_super_block(disk_path, 0);
}

void show_directory(uint32_t inode_id) {
    Inode inode = Inode::read_inode(inode_id);
    if (inode.i_type != DIR_TYPE) {
        return;
    }
    IndexBlock index_block = IndexBlock::read_index_block(inode.i_indirect);
    for (uint32_t i = 0; i < 254; ++i) { // 访问索引块
        if (index_block.index[i] == UINT32_MAX) {
            break;
        }
        DirBlock dir_block = DirBlock::read_dir_block(index_block.index[i]);
        std::cout << std::left << std::setw(10) << "name" << std::setw(10) << "mode" << std::setw(10) << "size" << std::setw(10) << "last change" << std::endl;

        for (int j = 0; j < 32; ++j) {
            if (dir_block.entries[j].type == UNDEFINE_TYPE) {
                continue;
            }
            Inode temp_inode = Inode::read_inode(dir_block.entries[j].inode_id);
            std::string print_name = std::string(dir_block.entries[j].name);
            if (dir_block.entries[j].type == DIR_TYPE) {
                print_name += "/";
            }
            std::cout << std::left << std::setw(10) << print_name << std::setw(10) << temp_inode.i_mode << std::setw(10) << temp_inode.i_size << std::setw(10) << format_time(temp_inode.i_mtime) << std::endl;
        }
    }
}

bool is_path_dir(const std::string &path, uint32_t &purpose_id) {
    // path 需要以 / 结尾，且不能包含 //
    if (path.find("//") != std::string::npos || path.back() != '/') {
        std::cerr << ERROR << "输入路径格式错误，请重新输入" << NORMAL << std::endl;
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
            std::cerr << ERROR << "目录不存在，请检查" << p << "是否正确" << NORMAL << std::endl;
            return false;
        }
    }

    purpose_id = temp_inodeid;
    return true;
}

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

bool is_file_exit(const std::string &name, Inode &cur_inode) {
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
                if (dir_block.entries[j].name == name) {
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

bool is_valid_dir_name(const std::string &dir_name) {
    // 目录不能包含 / ，长度不能超过28
    if (dir_name.find("/") != std::string::npos || dir_name.size() > 28) {
        return false;
    } else {
        return true;
    }
}

uint32_t make_dir_help(const std::string &dir_name, Inode &cur_inode) {
    IndexBlock cur_ib = IndexBlock::read_index_block(cur_inode.i_indirect);
    bool write_flag = false;
    Inode new_inode = {
        inode_bitmap.get_free_inode(),
        sizeof(DirBlock),
        2,
        1,
        block_bitmap.get_free_block(),
        DIR_TYPE,
        755,
        0,
        0,
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

std::string read_file(std::string file_path, std::string file_name) {
    uint32_t start_id = 0;
    if (!is_dir_exit(file_path, start_id)) {
        std::cout << ERROR << "目标目录" << file_path << "不存在" << NORMAL << std::endl;
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
            return "it is empty";
        }
        return file_content;
    } else {
        std::cout << ERROR << "目标文件" << file_name << "不存在" << NORMAL << std::endl;
        return "";
    }
}

bool write_file(std::string file_path, std::string file_name, std::string content) {
    uint32_t start_id = 0;
    if (!is_dir_exit(file_path, start_id)) {
        std::cout << ERROR << "目标目录" << file_path << "不存在" << NORMAL << std::endl;
        return false;
    }
    Inode dir_inode = Inode::read_inode(start_id);
    //向一个已经存在的文件后增加内容
    if (is_file_exit(file_name, dir_inode)) {
        Inode file_inode = Inode::read_inode(get_file_inode_id(file_name, dir_inode));
        IndexBlock file_ib = IndexBlock::read_index_block(file_inode.i_indirect);
        uint32_t block_id = UINT16_MAX;
        uint32_t file_size = file_inode.i_size;
        int need_num = (content.size() + file_size) / BLOCK_SIZE + 1;
        // 找到对应的块并写入
        for (int i = 0 ;i<need_num;i++){
            if (i<254){
                if (file_ib.index[i] == UINT32_MAX){
                    file_ib.index[i] = block_bitmap.get_free_block();
                }
                block_id = file_ib.index[i];
            }else{
                if (file_ib.next_index == UINT32_MAX){
                    file_ib.next_index = block_bitmap.get_free_block();
                }
                file_ib = IndexBlock::read_index_block(file_ib.next_index);
                block_id = file_ib.index[i-254];
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
        std::cout << ERROR << "目标文件" << file_name << "不存在" << NORMAL << std::endl;
        return false;
    }

   
}
// 输入一定是需要创建的目录
bool make_dir(const std::string dir_name, Inode cur_inode) {
    std::vector<std::string> path_list;
    std::string temp;
    for (auto c : dir_name) {
        if (c == '/' && !temp.empty()) {
            path_list.push_back(temp);
            if (!is_valid_dir_name(temp)) {
                std::cout << ERROR << "目录名" << temp << "不合法" << NORMAL << std::endl;
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
            cur_id = make_dir_help(p, cur_inode);
            cur_inode = Inode::read_inode(cur_id);
        }
    }
    return true;
}

bool make_file(const std::string file_name, uint32_t inode_id) {
    Inode parent_inode = Inode::read_inode(inode_id);
    if (!is_file_exit(file_name, parent_inode)) {
        IndexBlock cur_ib = IndexBlock::read_index_block(parent_inode.i_indirect);
        Inode new_inode = {
            inode_bitmap.get_free_inode(),
            0,
            0,
            1,
            block_bitmap.get_free_block(),
            FILE_TYPE,
            755,
            0,
            0,
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
        std::cout << ERROR << "文件" << file_name << "已存在" << NORMAL << std::endl;
        return false;
    }
}

bool del_file(const std::string file_name, Inode &cur_inode) {
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
        std::cout << ERROR << "文件" << file_name << "不存在" << NORMAL << std::endl;
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

bool del_dir(const uint32_t dir_inode_id) {
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
                    if (!del_dir(cur_db.entries[j].inode_id)) {
                        return false;
                    }
                } else if (cur_db.entries[j].type == FILE_TYPE) {
                    if (!del_file(cur_db.entries[j].name, dir_inode)) {
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
int main() {
    using namespace std;
    Inode root_inode, cur_inode;
    root_inode = cur_inode = Inode::read_inode(0);
    SuperBlock sb = SuperBlock::read_super_block();      // 读超级块
    uint32_t load_time = static_cast<uint32_t>(time(0)); // 保留登录时间
    std::string user = "root";
    std::string path = "/";

    while (1) {
        string input, cmd, tmp_arg;
        cout << USER << user + "@FileSystem" << NORMAL << ":" << PATH << path << NORMAL << "$ ";
        vector<string> args;
        std::getline(cin, input);
        std::istringstream iss(input);
        iss >> cmd;
        while (iss >> tmp_arg) {
            args.push_back(tmp_arg);
        }
        // 解析参数
        std::map<std::string, std::string> options;
        std::string arg;
        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i].front() == '-') {
                // 检查是否有参数
                if (i + 1 < args.size() && args[i + 1].front() != '-') {
                    options[args[i]] = args[i + 1];
                    ++i; // 跳过参数
                } else {
                    options[args[i]] = "true";
                }
            } else {
                // 将最后一个非选项参数赋值给 arg
                arg = args[i];
            }
        }
        if (arg.empty() && !args.empty()) {
            arg = args.back();
        }
        if (cmd == "exit") {
            break;
        } else if (cmd == "init") {
            init_disk();
            sb = SuperBlock::read_super_block();
        } else if (cmd == "info") {
            sb.save_super_block();
            sb.print_super_block();
        } else if (cmd == "cd") {
            uint32_t purpose_id = cur_inode.i_id;
            if (arg.empty()) { // no args ,into root
                cur_inode = root_inode;
                path = "/";
                continue;
            }
            if (arg.back() != '/') {
                arg += "/";
            }
            if (is_path_dir(arg, purpose_id)) { // 是一个目录
                cur_inode = Inode::read_inode(purpose_id);
                path = get_absolute_path(purpose_id);
            }
        } else if (cmd == "md") {
            // no args ,raise error
            if (arg.empty()) {
                cout << ERROR << "请输入目录名" << NORMAL << endl;
                continue;
            }
            if (arg.back() != '/') {
                arg += "/";
            }
            if (arg.find("//") != std::string::npos) {
                std::cout << ERROR << "输入路径格式错误，请重新输入" << NORMAL << std::endl;
                continue;
            }
            uint32_t start_id;
            // 绝对路径
            if (arg.front() == '/') {
                start_id = 0;
                if (is_dir_exit(arg, start_id)) {
                    std::cout << ERROR << "目录" << arg << "已存在" << NORMAL << std::endl;
                } else {
                    if (make_dir(arg, root_inode)) {
                        std::cout << SUCCESS << "目录" << arg << "创建成功" << NORMAL << std::endl;
                    }
                }
            }
            // 相对路径
            else {
                start_id = cur_inode.i_id;
                if (is_dir_exit(arg, start_id)) {
                    std::cout << ERROR << "目录" << arg << "已存在" << NORMAL << std::endl;
                } else {
                    if (make_dir(arg, cur_inode)) {
                        std::cout << SUCCESS << "目录" << arg << "创建成功" << NORMAL << std::endl;
                    }
                }
            }
        } else if (cmd == "rd") {
            // no args ,raise error
            if (arg.empty()) {
                cout << ERROR << "请输入目录名" << NORMAL << endl;
                continue;
            }
            if (arg.find("//") != std::string::npos) {
                std::cout << ERROR << "输入路径格式错误，请重新输入" << NORMAL << std::endl;
                continue;
            }
            if (arg.back() != '/') {
                arg += "/";
            }
            // 全部转换为绝对路径
            string dir_path = arg;
            uint32_t purpose_id = 0;
            if (dir_path.front() != '/') {
                dir_path = get_absolute_path(cur_inode.i_id) + dir_path;
            }
            if (is_dir_exit(dir_path, purpose_id)) {
                if (is_dir_empty(purpose_id) || options["-rf"] == "true") {
                    if (del_dir(purpose_id)) {
                        std::cout << SUCCESS << "目录" << dir_path << "删除成功" << NORMAL << std::endl;
                        if (cur_inode.i_id == purpose_id) {
                            cur_inode = root_inode;
                            path = "/";
                        }
                    } else {
                        std::cout << ERROR << "目录" << dir_path << "不为空" << NORMAL << std::endl;
                    }
                } else {
                    std::cout << ERROR << "目录" << dir_path << "不存在" << NORMAL << std::endl;
                }
            }
        } else if (cmd == "newfile") {
            // no args ,raise error
            if (arg.empty()) {
                cout << ERROR << "请输入文件名" << NORMAL << endl;
                continue;
            }
            // args , analyse path
            if (arg.find("//") != std::string::npos || arg.back() == '/') {
                std::cout << ERROR << "文件名输入错误，请重新输入" << NORMAL << std::endl;
                continue;
            }
            uint32_t start_id = 0;
            // 将args分为文件名和路径
            size_t pos = arg.find_last_of('/');
            string file_path, file_name;
            if (pos != std::string::npos) {
                file_path = arg.substr(0, pos + 1);
                file_name = arg.substr(pos + 1);
            } else {
                file_path = "";
                file_name = arg;
            }
            if (arg.front() != '/') {

                file_path = get_absolute_path(cur_inode.i_id) + file_path;
            }
            if (is_dir_exit(file_path, start_id)) { // 目录存在
                make_file(file_name, start_id);
            } else { // 目录不存在
                if (make_dir(file_path, root_inode)) {
                    is_dir_exit(file_path, start_id); // 找到目录
                    make_file(file_name, start_id);
                }
            }
        } else if (cmd == "cat") {
            // no args ,raise error
            if (arg.empty()) {
                cout << ERROR << "请输入文件名" << NORMAL << endl;
                continue;
            }
            // 将args分为文件名和路径
            size_t pos = arg.find_last_of('/');
            string file_path, file_name;
            if (pos != std::string::npos) {
                file_path = arg.substr(0, pos + 1);
                file_name = arg.substr(pos + 1);
            } else {
                file_path = "";
                file_name = arg;
            }
            if (arg.front() != '/') {
                file_path = get_absolute_path(cur_inode.i_id) + file_path;
            }
            // 读文件
            if (options["-i"].empty()) {
                std::string output = read_file(file_path, file_name);
                if (!output.empty()) {
                    std::cout << output << std::endl;
                }
            } else { // -i
                write_file(file_path, file_name, options["-i"]);
            }

        } else if (cmd == "copy") {
            // no args ,raise error
            if (arg.empty()) {
                cout << ERROR << "请输入文件名" << NORMAL << endl;
                continue;
            }
            // args , analyse path
        } else if (cmd == "del") {
            // no args ,raise error
            if (arg.empty()) {
                cout << ERROR << "请输入文件名" << NORMAL << endl;
                continue;
            }
            // args , analyse path
            if (arg.find("//") != std::string::npos || arg.back() == '/') {
                std::cout << ERROR << "文件名输入错误，请重新输入" << NORMAL << std::endl;
                continue;
            }
            uint32_t start_id = 0;
            // 将args分为文件名和路径
            size_t pos = arg.find_last_of('/');
            string file_path, file_name;
            if (pos != std::string::npos) {
                file_path = arg.substr(0, pos + 1);
                file_name = arg.substr(pos + 1);
            } else {
                file_path = "";
                file_name = arg;
            }
            if (arg.front() != '/') {

                file_path = get_absolute_path(cur_inode.i_id) + file_path;
            }
            if (is_dir_exit(file_path, start_id)) { // 目录存在
                Inode dir_inode = Inode::read_inode(start_id);
                if (is_file_exit(file_name, dir_inode)) {
                    del_file(file_name, dir_inode);
                }
            }
        } else if (cmd == "check") {
        } else if (cmd == "ls") {
            show_directory(cur_inode.i_id);
        } else if (cmd == "test") {
            std::string base_path = get_absolute_path(4);
            // 调试
            // 输出inodemap的bitset
            int i = 0;
            while (i < INODE_COUNT) {
                if (block_bitmap.bitmap[i]) {
                    cout << i << " ";
                }
                i++;
            }
        } else if (cmd == "clear") {
            system("cls");
        } else {
            cout << ERROR << "未定义的命令，请重新输入" << NORMAL << endl;
            continue;
        }
    }
    // 退出程序前保存超级块
    sb.last_load_time = load_time;
    sb.save_super_block();
}
