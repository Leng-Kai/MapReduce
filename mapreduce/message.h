#ifndef MESSAGE_H
#define MESSAGE_H

#define IDLE		"idle"
#define M_FINISH	"m_finish"	/* worker -> master */
#define R_FINISH	"r_finish"

#define M_TASK		"m_task"
#define R_TASK		"r_task"	/* master -> worker */
#define NO_TASK		"no_task"
#define TERMINATE	"terminate"

#define MAX_PAYLOAD_SIZE	1024

#include <string>
#include <vector>
using namespace std;

string gen_payload(string msg_type, vector<string> &msgs) {
	string payload = msg_type + "#";
	for (size_t i = 0; i < msgs.size(); i++) {
		payload += msgs[i];
		if (i != msgs.size() - 1)
			payload += " ";
	}
	return payload;
}

void parse_payload(string payload, string &msg_type, vector<string> &msg) {
	size_t pos = payload.find_first_of("#");
	msg_type = payload.substr(0, pos);
	payload = payload.substr(pos + 1);

	while (payload.size()) {
		pos = min(payload.find_first_of(" "), payload.size());
		msg.push_back(payload.substr(0, pos));
		payload = payload.substr(min(pos + 1, payload.size()));
	}
}

#endif /* MESSAGE_H */
