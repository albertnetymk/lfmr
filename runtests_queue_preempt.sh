#!/bin/sh

timestamp=`date +%Y.%m.%d.%H.%M.%S`
dir=$0.$timestamp
mkdir $dir
cat /proc/cpuinfo > $dir/cpuinfo

queuesize=10
duration=1000

for i in test_queue_spinlock.exe \
    test_queue_lf_qsbr.exe test_queue_lf_ebr.exe \
    test_queue_lf_smr.exe test_queue_lf_nebr.exe
do
  echo "Testing $i"

  # Graph with respect to number of threads.
  for ncpu in 1 2 4 7 8 9 16 24 32
    do
    ./$i $duration $queuesize $ncpu
  done > $dir/$i.nq${nq}

done
