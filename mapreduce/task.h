#ifndef TASK_H
#define TASK_H

#include <fstream>
#include <string>
#include <unordered_map>
#include <functional>
using namespace std;

#define gen_file_macro(_1, _2, f, ...) f
#define gen_file_name(...) gen_file_macro(__VA_ARGS__, gf2, gf1)(__VA_ARGS__)
#define gf2(m, r)       "w" + to_string(worker_num) + "_m" + m + "_r" + r
#define gf1(r)          "w" + to_string(worker_num) + "_r" + r

/*
 *  function: mapper
 *
 *  parameters:
 *  	ifstream &ifs: 				file input
 *  	int r_task_total:			number of reduce tasks
 *
 *  	function<void(int, string)> emit_to:	emit function
 *
 *  description:
 *  	e.g. emit_to(3, "ABC") output the string "ABC" (with a newline added)
 *	to the input file of reduce task 3 (The reduce tasks are 0-indexed).
 */

void mapper(ifstream &ifs, int r_task_total,
	function<void(int, string)> emit_to) {
	string str;
	while (ifs >> str) {
		char c = str[0];
		if (!(c >= 'A' && c <= 'Z') && !(c >= 'a' && c <= 'z'))
			str = str.substr(1);
		c = str[str.size() - 1];
		if (!(c >= 'A' && c <= 'Z') && !(c >= 'a' && c <= 'z'))
			str = str.substr(0, str.size() - 1);
		for (char &c : str)
			c = toupper(c);
		emit_to(hash<string>{}(str) % r_task_total, str);
	}
}

/*
 *  function: reducer
 *
 *  parameters:
 *  	ifstream &ifs:	file input
 *  	ofstream &ofs:	file output
 */

void reducer(ifstream &ifs, ofstream &ofs) {
	unordered_map<string, int> word_freq;
	string str;
	while (ifs >> str) {
		if (!word_freq.count(str))
			word_freq[str] = 0;
		word_freq[str]++;
	}
	for (auto &pair : word_freq)
		ofs << pair.first + " " + to_string(pair.second) << "\n";
}

#endif /* TASK_H */
