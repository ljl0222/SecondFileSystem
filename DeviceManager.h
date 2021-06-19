#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <cstdio>

using namespace std;

class DeviceManager {
public:
    /* 磁盘镜像文件名 */
    static const char* DISK_FILE_NAME;

private:
    /* 磁盘文件指针 */
    FILE* fp;

public:
    DeviceManager();
    ~DeviceManager();

    /* 检查镜像文件是否存在 */
    bool Exists();

    /* 打开镜像文件 */
    void Construct();

    /* 实际写磁盘函数 */
    void write(const void* buffer, unsigned int size,
        int offset = -1, unsigned int origin = SEEK_SET);

    /* 实际写磁盘函数 */
    void read(void* buffer, unsigned int size,
        int offset = -1, unsigned int origin = SEEK_SET);
};

#endif