#ifndef TASK_WRAPPER_H
#define TASK_WRAPPER_H

#include <fstream>
#include <string>
#include <vector>
#include <functional>
#include "task.h"
using namespace std;

extern int      worker_num;

void mapper(ifstream &ifs, int r_task_total,
		function<void(int, string)> emit_to);
void reducer(ifstream &ifs, ofstream &ofs);

void __emit_to(vector<ofstream> &ofs,
		int r_task_id, string data) {
	ofs[r_task_id] << data << "\n";
}

void do_m_task(vector<string> &recv_msgs, vector<string> &send_msgs) {
	using namespace std::placeholders;

	string m_task_id = recv_msgs[0];
	string input_file = recv_msgs[1];
	int r_task_total = stoi(recv_msgs[2]);

	send_msgs.push_back(m_task_id);

	ifstream ifs(input_file, ios::in);
	vector<ofstream> ofs_list;
	for (int i = 0; i < r_task_total; i++) {
		string output_file = gen_file_name(m_task_id, to_string(i));
		ofstream ofs(output_file, ios::out);
		ofs_list.push_back(move(ofs));
		send_msgs.push_back(output_file);
	}

	cout << "worker " << worker_num << " running map task " << m_task_id << "\n";
	mapper(ifs, r_task_total, std::bind(__emit_to, ref(ofs_list), _1, _2));

	ifs.close();
	for (auto &ofs : ofs_list)
		ofs.close();
}

void do_r_task(vector<string> &recv_msgs, vector<string> &send_msgs) {
	string r_task_id = recv_msgs[0];
	string input_file = recv_msgs[1];

	send_msgs.push_back(r_task_id);

	string output_file = gen_file_name(r_task_id);
	send_msgs.push_back(output_file);
	ifstream ifs(input_file, ios::in);
	ofstream ofs(output_file, ios::out);

	cout << "worker " << worker_num << " running reduce task " << r_task_id << "\n";
	reducer(ifs, ofs);
}

#endif /* TASK_WRAPPER_H */

