# 读取二进制文件
import time
with open('Disk\MyDisk.dat', 'rb') as f:
    start_time = time.time()
    data = f.read()
    end_time = time.time()
    print(data)  

print('读取文件耗时：', end_time - start_time)