# 读取二进制文件
import time
with open('Disk\\MyDisk.dat', 'rb') as f:
    start_time = time.time()
    data = f.read()
    end_time = time.time()
    print(data)  

print('读取文件耗时：', end_time - start_time)std::string welcome1 = 
"    _______ __        _____            __                \n"
"   / ____(_/ ___     / ___/__  _______/ /____  ____ ___  \n"
"  / /_  / / / _ \\    \\__ \\/ / / / ___/ __/ _ \\/ __ `__ \\ \n"
" / __/ / / /  __/   ___/ / /_/ (__  / /_/  __/ / / / / / \n"
"/_/   /_/_/\\___/   /____/\\__, /____/\\__/\\___/_/ /_/ /_/  \n"
"                        /____/                           \n";