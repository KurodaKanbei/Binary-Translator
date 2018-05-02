#include <bits/stdc++.h>

using namespace std;


class parser {

	public:

	ifstream input;
	
	string text[256];
	
	int cnt;
	

	string substr(char* str, int pos, int len) {
		string ret = "";	
		for (int i = pos; i < pos + len; i++) {
			ret = ret + str[i];
		}
		return ret;
	}

	bool exist(char* str, string s) {
		int n = strlen(str);
		int m = s.length();
		for (int i = 0; i + m <= n; i++) {
			if (substr(str, i, m) == s) {
				return true;
			}
		}
		return false;
	}

	string process(char* str) {
		int n = strlen(str);
		if (exist(str, ".text") || exist(str, ".align")) return substr(str, 0, n);
		if (exist(str, ".rodata")) return "\t.data";
		if (exist(str, ".long")) return "\t.word";
		if (str[0] != '\t') return substr(str, 0, n);
		if (str[n - 1] == '.') return substr(str, 0, n);
		if (str[0] == '\t') {
			if (str[1] != '.') return substr(str, 0, n);
			return "";	
		}
		return "";
	}
	
	parser(string filename) {
		input.open(filename);
		char buffer[256];
		cnt = 0;
		for (;;) {
			input.getline(buffer, 256);
			int m = strlen(buffer);
			if (m == 0) break;
			string str = process(buffer);
			if (str != "") {
				text[cnt++] = str;
			}
		}
		for (int i = 0; i < cnt; i++) {
			cerr << text[i] << endl;
		}
	}
};
