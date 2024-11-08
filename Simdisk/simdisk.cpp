#include<iostream>
#include<fstream>
//超级块类，包含磁盘的基本信息
//包括inode数，block数，根目录信息等
class SuperBlock{
public:
    SuperBlock(){
        block_size = 1024;      //块大小默认为1KB
        total_block_num = 102400; 
        remain_block_num = 102400;
        total_size = block_size * total_block_num; //总大小
    }
    void show(){
        std::cout<<"block_size: "<<block_size<<std::endl;
        std::cout<<"total_block_num: "<<total_block_num<<std::endl;
        std::cout<<"remain_block_num: "<<remain_block_num<<std::endl;
        std::cout<<"total_size: "<<total_size<<std::endl;
    }

    //初始化超级块
    int block_size;             //块大小,单位为字节
    int total_block_num;        //总块数
    int remain_block_num;       //剩余块数
    int total_inode_num;        //总inode数
    int remain_inode_num;       //剩余inode数
    int root_dir_inode;         //根目录inode号
    int total_size;             //总大小


};


void init_disk(){

}
int main(){
    SuperBlock super_block;
    super_block.show();
    super_block.save_super_block("Disk\\MyDisk.dat");
    return 0;
}