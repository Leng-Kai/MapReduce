CXX = g++
CPPFLAGS = 
CXXFLAGS = -std=c++17 -Wall

.default: all

all: master worker

master: mapreduce/master.h mapreduce/message.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -pthread -o master master.cpp

worker: mapreduce/message.h mapreduce/task_wrapper.h mapreduce/task.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o worker worker.cpp

clean:
	$(RM) master worker
