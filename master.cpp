#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>
#include "mapreduce/master.h"
using namespace std;

int ret;

struct sockaddr_in servaddr;
int connection;

int m_task = 10;
int r_task = 10;
string file;
int port = 5057;

int main(int argc, char *argv[]) {
	int opt;
	while ((opt = getopt(argc, argv, "m:r:d:p:")) != -1) {
		switch (opt) {
		case 'm':
			m_task = stoi(optarg);
			break;
		case 'r':
			r_task = stoi(optarg);
			break;
		case 'd':
			file = optarg;
			break;
		case 'p':
			port = stoi(optarg);
			break;
		}
	}

	cout << m_task << " map tasks, " << r_task << " reduce tasks.\n";
	cout << "input file: " << file << "\n";

	Master master(m_task, r_task, file);
	ret = master.listen_on(port);

	cout << "Finished.\n";
	master.timing_info();

	return 0;
}
