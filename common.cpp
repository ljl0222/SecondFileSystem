#include "common.h"
#include <iostream>
#include <sstream>

vector<string> getCmd(string str)
{
	vector<string> result;
	result.clear();

	stringstream cmd(str); // ��str��Ϊ������
	string tempResult;
	while (cmd >> tempResult) {
		result.push_back(tempResult);
	}

	if (DEBUG) {
		cout << "����������������Ϊ" << endl;
		for (int cnt = 0; cnt < result.size(); cnt++) {
			cout << result[cnt] << ' ';
		}
		cout << endl;
	}

	return result;
}