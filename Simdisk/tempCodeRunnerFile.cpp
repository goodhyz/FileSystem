    //考虑对齐与填充，类型都设置为uint32_t
    uint32_t i_size;        // 文件大小, 以字节为单位
    uint32_t i_blocks;      // 文件所占数据块数量
    uint32_t i_links_count; // 链接数

    /*访问控制*/
    uint32_t i_mode;        // 文件权限和类型 9位权限 + 3位文件类型
    uint32_t i_uid;         // 用户 ID
    uint32_t i_gid;         // 组 ID
    /*时间戳*/
    uint32_t i_ctime;       // 创建时间
    uint32_t i_mtime;       // 修改时间
    uint32_t i_atime;       // 访问时间

    uint32_t i_indirect;    // 一级间接块指针 0-102399
    uint32_t i_flags;       // 文件系统标志