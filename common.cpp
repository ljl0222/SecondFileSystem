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