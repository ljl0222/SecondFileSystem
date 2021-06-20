
#include "common.h"
#include <iostream>

bool DEBUG;


DeviceManager globalDeviceManager;
BufferManager globalBufferManager;
OpenFileTable globalOpenFileTable;
SuperBlock globalSuperBlock;
FileSystem globalFileSystem;
InodeTable globalINodeTable;
FileManager globalFileManager;
User globalUser;

string line;

int main()
{
	DEBUG = false;


	while (1) {
		cout << "[u1851977@localhost " + globalUser.u_curdir + "]$ ";
		getline(cin, line);

		vector<string> cmdList = getCmd(line); // ��ʱ��һ����Ϊ������

		exeCmd(cmdList);

	}

	return 0;
}