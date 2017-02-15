#!/bin/sh

timestamp=`date +%Y.%m.%d.%H.%M.%S`
dir=$0.$timestamp
mkdir $dir
cat /proc/cpuinfo > $dir/cpuinfo

queuesize=10
nbuckets=100
duration=1000

for i in test_hash_lf_smr.exe test_hash_lf_qsbr.exe \
    test_hash_cr_smr.exe test_hash_cr_qsbr.exe
do
  echo "Testing $i"

  # Test scalability with respect to update fraction
  for ncpu in 1 4 7
    do
    for elem in 1
    do
      for u in 0 1 2 5 10 25 50 75 100
        do
        ./$i $duration $u/100 $((${elem}*${nbuckets})) $nbuckets $ncpu
      done > $dir/$i.cpu${ncpu}.elem${elem}
    done
  done

done
