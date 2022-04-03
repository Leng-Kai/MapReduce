# MapReduce
A local multi-process MapReduce implementation written in C++

The mapper and reducer functions are implemented in `mapreduce/task.h`, they are wrapped by the wrapper functions implemented in `mapreduce/task_wrapper.h`, so that the users can focus only on the implementation of mapper and reducer, but not the details of writting files and sending messages. The `mapreduce/task.h` file already contains an example implementation of mapper and reducer for word count application.

To run the MapReduce system for word count, first compile the master and worker:
```
# make
```

Run the master:
```
# ./master -d <file> -m <map task> -r <reduce task> -p <port>
```

with the below arguments provided:
```
  -d <file>         the file input for the task (required)
  -m <map task>     the number of map tasks (default 10)
  -r <reduct task>  the number of reduce tasks (default 10)
  -p <port>         the port worker listens on (default 5057)
```

For example, to run the master for word count with the provided example test data, run:
```
# ./master -d data.txt
```

The master will then listen on port 5057 and wait for the work request from workers.

After running up the master, we can run multiple workers to complete the map tasks and reduce tasks concurrently. A shell script `run-workers.sh` is provided to easily execute multiple worker processes:
```
# ./run-workers.sh <workers> <port>
```

The first argument should be the number of workers (default 8), and the second argument should be the port for connecting to the master (default 5057). If the master does run on the default port, this argument should be specified to the same port so that the workers can reach the master.

For example, to run the workers for word count, run:
```
# ./run-workers.sh
```

The workers will then send requests for tasks to master, and complete the tasks concurrently. After all tasks are completed, the master response the workers with termination message so that the workers can terminate, and the master itself also terminates after all workers are terminated.

The result will be written in `result.txt`:
```
GENERICIZED 1
OR 1
INFRASTRUCTURE 1
ALSO 1
COMMUNICATION 2
BENEFICIAL 1
THIS 1
ENGINE 1
...
```
