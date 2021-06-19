#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <cstdio>

using namespace std;

class DeviceManager {
public:
    /* ���̾����ļ��� */
    static const char* DISK_FILE_NAME;

private:
    /* �����ļ�ָ�� */
    FILE* fp;

public:
    DeviceManager();
    ~DeviceManager();

    /* ��龵���ļ��Ƿ���� */
    bool Exists();

    /* �򿪾����ļ� */
    void Construct();

    /* ʵ��д���̺��� */
    void write(const void* buffer, unsigned int size,
        int offset = -1, unsigned int origin = SEEK_SET);

    /* ʵ��д���̺��� */
    void read(void* buffer, unsigned int size,
        int offset = -1, unsigned int origin = SEEK_SET);
};

#endif