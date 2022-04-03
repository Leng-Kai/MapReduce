#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <vector>
#include <iostream>
#include "mapreduce/message.h"
#include "mapreduce/task_wrapper.h"
using namespace std;

int 	ret;

int	sd;
string	ip = "127.0.0.1";
int	port = 5057;

int	worker_num = 0;

int main(int argc, char *argv[]) {
	int opt;
	while ((opt = getopt(argc, argv, "n:p:")) != -1) {
		switch (opt) {
		case 'n':
			worker_num = stoi(optarg);
			break;
		case 'p':
			port = stoi(optarg);
			break;
		}
	}

	vector<string> v = { to_string(worker_num) };
	string req = gen_payload(IDLE, v);

	while (true) {
		if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
			cerr << "\nSocket creation error.\n";
			return -1;
		}

		struct sockaddr_in remaddr;
		bzero(&remaddr, sizeof(remaddr));
		remaddr.sin_family = PF_INET;
		remaddr.sin_addr.s_addr = inet_addr(ip.c_str());
		remaddr.sin_port = htons(port);

		if ((ret = connect(sd, (struct sockaddr *)&remaddr,
						sizeof(remaddr))) < 0) {
			cerr << "\nConnection error.\n";
			return -1;
		}

		send(sd, req.c_str(), req.size(), 0);
		char buf[MAX_PAYLOAD_SIZE] = {0};
		recv(sd, buf, MAX_PAYLOAD_SIZE, 0);

		string send_msg_type, recv_msg_type;
		vector<string> send_msgs, recv_msgs;
		parse_payload((string)buf, recv_msg_type, recv_msgs);
		send_msgs.push_back(to_string(worker_num));

		if (recv_msg_type == NO_TASK) {
			send_msg_type = IDLE;
		} else if (recv_msg_type == M_TASK) {
			send_msg_type = M_FINISH;
			do_m_task(recv_msgs, send_msgs);
		} else if (recv_msg_type == R_TASK) {
			send_msg_type = R_FINISH;
			do_r_task(recv_msgs, send_msgs);
		} else if (recv_msg_type == TERMINATE) {
			break;
		}

		req = gen_payload(send_msg_type, send_msgs);

		close(sd);
	}

	close(sd);
	return 0;
}
