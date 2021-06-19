#define _CRT_SECURE_NO_WARNINGS
#include "Buf.h"
#include "BufferManager.h"
#include "DeviceManager.h"
#include "File.h"
#include "FileManager.h"
#include "FileSystem.h"
#include "INode.h"
#include "OpenFileManager.h"
#include "User.h"
using namespace std;


const char* DeviceManager::DISK_FILE_NAME = "FS.img";

DeviceManager::DeviceManager() {
    fp = fopen(DISK_FILE_NAME, "rb+");
}

DeviceManager::~DeviceManager() {
    if (fp) {
        fclose(fp);
    }
}

/* ��龵���ļ��Ƿ���� */
bool DeviceManager::Exists() {
    return fp != NULL;
}

/* �򿪾����ļ� */
void DeviceManager::Construct() {
    fp = fopen(DISK_FILE_NAME, "wb+");
    if (fp == NULL) {
        printf("�򿪻��½��ļ�%sʧ�ܣ�", DISK_FILE_NAME);
        exit(-1);
    }
}

/* ʵ��д���̺��� */
void DeviceManager::write(const void* buffer, unsigned int size, int offset, unsigned int origin) {
    if (offset >= 0) {
        fseek(fp, offset, origin);
    }
    fwrite(buffer, size, 1, fp);
}

/* ʵ��д���̺��� */
void DeviceManager::read(void* buffer, unsigned int size, int offset, unsigned int origin) {
    if (offset >= 0) {
        fseek(fp, offset, origin);
    }
    fread(buffer, size, 1, fp);
}