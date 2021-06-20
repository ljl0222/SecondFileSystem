#include "common.h"
#include <iostream>
#include <sstream>

vector<string> getCmd(string str)
{
	vector<string> result;
	result.clear();

	stringstream cmd(str); // 把str作为流输入
	string tempResult;
	while (cmd >> tempResult) {
		result.push_back(tempResult);
	}

	if (DEBUG) {
		cout << "读到的命令行内容为" << endl;
		for (int cnt = 0; cnt < result.size(); cnt++) {
			cout << result[cnt] << ' ';
		}
		cout << endl;
	}

	return result;
}

void exeCmd(vector<string> cmdList)
{
	if (cmdList.size() == 0) {
		return;
	}

	string cmd = cmdList[0];

	if (cmd == "format") {
		cout << "格式化整个磁盘空间，请重启程序" << endl;
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
		return;
	}
	else {
		cout << "未识别到的符号: " << cmd << endl;
	}
}