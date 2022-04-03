#ifndef MASTER_H
#define MASTER_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <ctime>
#include <thread>
#include <mutex>
#include <functional>
#include <future>
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_set>
#include "message.h"
using namespace std;
using chrono::time_point;
using chrono::duration_cast;
using chrono::milliseconds;
using chrono::system_clock;

#define MAX_CONNECTION		64

#define map_file_name(i) 	"m" + to_string(i)
#define reduce_file_name(i)	"r" + to_string(i)
#define result_file_name()	"result.txt"

#define time(nullptr)		chrono::high_resolution_clock::now();

enum { UNSCHEDULED, IN_PROGRESS, DONE };	/* task status */
enum { MAP_PHASE, REDUCE_PHASE, ENDING_PHASE };	/* current phase */

class Master
{
public:
	Master(int m_task_total, int r_task_total, string file) {
		this->m_task_total = m_task_total;
		this->r_task_total = r_task_total;
		this->input_file = file;
		this->m_task_done = this->r_task_done = 0;

		this->to_map_phase();
		this->current_phase = MAP_PHASE;
	}

	~Master() {}

	int listen_on(int port) {
		int listenfd, connfd;

		if ((listenfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
			cerr << "\nSocket creation error.\n";
			return -1;
		}

		struct sockaddr_in servaddr;
		int addrlen = sizeof(servaddr);
		bzero(&servaddr, sizeof(servaddr));
		servaddr.sin_family = PF_INET;
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servaddr.sin_port = htons(port);

		if (::bind(listenfd, (struct sockaddr*)&servaddr, 
					sizeof(servaddr)) < 0) {
			cerr << "\nbind socket error.\n";
			return -1;
		}

		if (listen(listenfd, MAX_CONNECTION) < 0) {
			cerr << "\nlisten socket error.\n";
			return -1;
		}

		cout << "listening on port " << port << ".\n";

		while (current_phase != ENDING_PHASE || !workers.empty()) {
			connfd = accept(listenfd,
					(struct sockaddr*) &servaddr,
					(socklen_t*) &addrlen);
			if (connfd < 0) {
				cerr << "\naccept socket error.\n";
				return -1;
			}

			future<void> fut = async(launch::async,
					bind(&Master::handle_worker_request,
						this, placeholders::_1),
					connfd);
		}

		close(listenfd);
		return 0;
	}

	void timing_info() {
		auto m_start = m_tasks[0].start;
		auto m_end = m_tasks[0].end;
		auto r_start = r_tasks[0].start;
		auto r_end = r_tasks[0].end;

		for (auto &m : m_tasks) {
			m_start = min(m_start, m.start);
			m_end = max(m_end, m.end);
		}
		for (auto &r : r_tasks) {
			r_start = min(r_start, r.start);
			r_end = max(r_end, r.end);
		}

		auto m_duration = duration_cast<milliseconds>(m_end - m_start);
		auto r_duration = duration_cast<milliseconds>(r_end - r_start);
		cout << "Timing Information:\n";
		cout << "Map Phase:    " << m_duration.count() << " msec\n";
		cout << "Reduce Phase: " << r_duration.count() << " msec\n";
	}

private:
	int current_phase;

	typedef struct {
		string file;
		chrono::time_point<chrono::high_resolution_clock> start, end;
		int status;
	} m_task;

	typedef struct {
		vector<string> files;
		string file;
		chrono::time_point<chrono::high_resolution_clock> start, end;
		int status;
	} r_task;

	int m_task_total, r_task_total;
	int m_task_done, r_task_done;

	vector<m_task> m_tasks;
	vector<r_task> r_tasks;

	unordered_set<int> workers;

	string input_file;
	vector<string> result_files;
	string result_file;

	mutex mtx;

	void to_map_phase() {
		ifstream ifs(input_file, ios::in);
		vector<ofstream> ofs_list;
		for (int i = 0; i < m_task_total; i++) {
			string output_file = map_file_name(i);
			ofstream ofs(output_file, ios::out);
			ofs_list.push_back(move(ofs));
			m_task m = {
				.file = output_file,
				.status = UNSCHEDULED,
			};
			m_tasks.push_back(m);
		}

		int i = 0;
		string line;
		while (getline(ifs, line)) {
			ofs_list[i] << line << "\n";
			i = (i + 1) % m_task_total;
		}

		for (auto &ofs : ofs_list)
			ofs.close();

		for (int i = 0; i < r_task_total; i++) {
			r_task r = {
				.files = vector<string>(),
				.status = UNSCHEDULED,
			};
			r_tasks.push_back(r);
		}
	}

	void to_reduce_phase() {
		for (int i = 0; i < r_task_total; i++) {
			string output_file = reduce_file_name(i);
			ofstream ofs(output_file, ios::out);
			ofs.close();
			for (auto &file : r_tasks[i].files) {
				ofstream ofs(output_file, ios::out | ios::app);
				ifstream ifs(file, ios::in);
				ofs << ifs.rdbuf();
				ifs.close();
				ofs.close();
			}
			r_tasks[i].file = output_file;
		}
	}

	void to_ending_phase() {
		string output_file = result_file_name();
		ofstream ofs(output_file, ios::out);
		ofs.close();
		for (auto &file : result_files) {
			ofstream ofs(output_file, ios::out | ios::app);
			ifstream ifs(file, ios::in);
			ofs << ifs.rdbuf();
			ifs.close();
			ofs.close();
		}
		result_file = output_file;
		cout << "Result has been written into " << result_file << ".\n";
	}

	void m_finish(vector<string> &recv_msgs) {
		int m_task_num = stoi(recv_msgs[0]);
		m_tasks[m_task_num].end = time(nullptr);
		m_tasks[m_task_num].status = DONE;
		
		for (int i = 0; i < r_task_total; i++)
			r_tasks[i].files.push_back(recv_msgs[i + 1]);

		if (++m_task_done == m_task_total) {
			to_reduce_phase();
			current_phase = REDUCE_PHASE;
		}
	}

	void r_finish(vector<string> &recv_msgs) {
		int r_task_num = stoi(recv_msgs[0]);
		r_tasks[r_task_num].end = time(nullptr);
		r_tasks[r_task_num].status = DONE;

		result_files.push_back(recv_msgs[1]);

		if (++r_task_done == r_task_total) {
			to_ending_phase();
			current_phase = ENDING_PHASE;
		}
	}

	bool m_get_task(vector<string> &send_msgs) {
		for (int i = 0; i < m_task_total; i++) {
			if (m_tasks[i].status == UNSCHEDULED) {
				send_msgs.push_back(to_string(i));
				send_msgs.push_back(m_tasks[i].file);
				send_msgs.push_back(to_string(r_task_total));
				m_tasks[i].status = IN_PROGRESS;
				m_tasks[i].start = time(nullptr);
				return true;
			}
		}

		return false;
	}

	bool r_get_task(vector<string> &send_msgs) {
		for (int i = 0; i < r_task_total; i++) {
			if (r_tasks[i].status == UNSCHEDULED) {
				send_msgs.push_back(to_string(i));
				send_msgs.push_back(r_tasks[i].file);
				r_tasks[i].status = IN_PROGRESS;
				r_tasks[i].start = time(nullptr);
				return true;
			}
		}

		return false;
	}

	void handle_worker_request(int connfd) {
		char buf[MAX_PAYLOAD_SIZE] = {0};

		recv(connfd, buf, MAX_PAYLOAD_SIZE, 0);

		string recv_msg_type, send_msg_type;
		vector<string> recv_msgs, send_msgs;

		parse_payload((string)buf, recv_msg_type, recv_msgs);

		mtx.lock();

		int worker_num = stoi(recv_msgs[0]);
		recv_msgs.erase(recv_msgs.begin());

		if (recv_msg_type == IDLE)
			workers.insert(worker_num);
		else if (recv_msg_type == M_FINISH)
			m_finish(recv_msgs);
		else if (recv_msg_type == R_FINISH)
			r_finish(recv_msgs);

		switch (current_phase) {
		case MAP_PHASE:
			send_msg_type = m_get_task(send_msgs) ? 
				M_TASK : NO_TASK;
			break;
		case REDUCE_PHASE:
			send_msg_type = r_get_task(send_msgs) ?
				R_TASK : NO_TASK;
			break;
		case ENDING_PHASE:
			send_msg_type = TERMINATE;
			workers.erase(worker_num);
			break;
		}

		string res = gen_payload(send_msg_type, send_msgs);
	
		mtx.unlock();

		send(connfd, res.c_str(), res.size(), 0);
		close(connfd);
	}






};

#endif /* MASTER_H */
