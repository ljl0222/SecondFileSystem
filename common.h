#ifndef COMMON_H
#define COMMON_H

#include "User.h"
#include "Buf.h"
#include "BufferManager.h"
#include "FileSystem.h"
#include "FileManager.h"
#include "OpenFileManager.h"
#include "DeviceManager.h"

#include <vector>

using namespace std;

extern bool DEBUG;

extern User globalUser;
extern DeviceManager globalDeviceManager;
extern BufferManager globalBufferManager;
extern OpenFileTable globalOpenFileTable;
extern SuperBlock globalSuperBlock;
extern FileSystem globalFileSystem;
extern InodeTable globalINodeTable;
extern FileManager globalFileManager;

// ���������������
vector<string> getCmd(string str);

// ���ж�Ӧ������
void exeCmd(vector<string> cmdList);

#endif