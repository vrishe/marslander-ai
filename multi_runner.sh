#!/bin/bash

config=$1
proc_count=$2
shift 2

bin_path="./build/$config/runner"
make check runner BUILD_CONFIG=$config

trap terminate SIGINT
terminate(){
  pkill -SIGKILL -P $$
  exit
}

while [ $proc_count -gt 0 ]; do
  proc_count=$((proc_count - 1));
  "$bin_path" ${@//%P/$proc_count} &
done

wait
