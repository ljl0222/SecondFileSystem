
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

	cout << "start!" << endl;

	while (1) {
		cout << "[" + globalUser.u_curdir + "]$ ";
		getline(cin, line);

		vector<string> cmdList = getCmd(line); // 此时第一项总为操作数

		if (cmdList.size() == 0) {
			continue;
		}

		string cmd = cmdList[0];

		if (cmd == "format") {
			globalOpenFileTable.Format();
			globalINodeTable.Format();
			globalBufferManager.Bformat();
			globalFileSystem.FormatFileSystem();
			exit(0);
		}
		else if (cmd == "creat") {
			globalUser.Mkdir(cmdList[1]);
		}
		else if (cmd == "open") {
			
		}
		else if (cmd == "read") {

		}
		else if (cmd == "write") {

		}
		else if (cmd == "seek") {

		}
		else if (cmd == "close") {

		}
		else if (cmd == "mkdir") {
			globalUser.Mkdir(cmdList[1]);
		}
		else if (cmd == "rm") {

		}
		else if (cmd == "ls") {
			globalUser.Ls();
		}
		else if (cmd == "cd") {
			globalUser.Cd(cmdList[1]);
		}
		else if (cmd == "exit") {
			exit(0);
		}
		else if (cmd == "") {
			continue;
		}
		else {
			cout << "未识别到的符号: " << cmd << endl;
		}

	}
}