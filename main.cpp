
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
			globalUser.Open(cmdList[1], cmdList[2]);
		}
		else if (cmd == "read") {
			if (cmdList[2] == "-o") {
				globalUser.Read(cmdList[1], cmdList[3], cmdList[4]);
			}
			else {
				globalUser.Read(cmdList[1], "", cmdList[2]);
			}
		}
		else if (cmd == "write") {
			globalUser.Write(cmdList[1], cmdList[2], cmdList[3]);
		}
		else if (cmd == "seek") {
			globalUser.Seek(cmdList[1], cmdList[2], cmdList[3]);
		}
		else if (cmd == "close") {
			globalUser.Close(cmdList[1]);
		}
		else if (cmd == "mkdir") {
			globalUser.Mkdir(cmdList[1]);
		}
		else if (cmd == "rm") {
			globalUser.Delete(cmdList[1]);
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