#!/bin/bash

WORKER_NUM=${1-8}
PORT=${2-5057}

echo "Running $WORKER_NUM workers"

for i in `seq 1 $WORKER_NUM`;
do
	./worker -n $i -p $PORT &
done
