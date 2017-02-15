#!/bin/sh

timestamp=`date +%Y.%m.%d.%H.%M.%S`
dir=$0.$timestamp
mkdir $dir
cat /proc/cpuinfo > $dir/cpuinfo

queuesize=10
nbuckets=100
duration=1000

for i in test_list_lf_lfrc.exe test_list_cr_lfrc.exe
do
  echo "Limited test of reference counting : $i"
  
  # Test scalability with multiple elements
  # Centralized freelist for reference counting; therefore, only test with
  # a read-only workload.
  for ncpu in 1 2 4 8 16 32
    do
    for u in 0
      do
      for elem in 0 1 5 10 25 50 100
       do
       ./$i $duration ${u}/100 $elem $ncpu
      done > $dir/$i.cpu${ncpu}.u${u}
    done
  done
done

for i in test_list_spinlock.exe \
    test_list_lf_smr.exe test_list_lf_qsbr.exe \
    test_list_lf_ebr.exe test_list_lf_nebr.exe \
    test_list_cr_smr.exe test_list_cr_qsbr.exe \
    test_list_cr_ebr.exe test_list_cr_nebr.exe \
    test_list_lf_hybrid_qsbr.exe test_list_lf_hybrid_nebr.exe
do
  echo "Testing $i"

  # Test scalability with respect to update fraction
  for ncpu in 1 2 4 8
    do
    for elem in 1 10 50 100
    do
      for u in 0 1 2 5 10 25 50 75 100
	do
	./$i $duration $u/100 $elem $ncpu
      done > $dir/$i.cpu${ncpu}.elem${elem}
    done
  done

  # Test scalability with multiple threads
  for elem in 1 10 50 100
    do
    for u in 0 10 100
      do
      for ncpu in 1 2 4 8 16 32
	do
	./$i $duration $u/100 $elem $ncpu
      done > $dir/$i.elem${elem}.u${u}
    done
  done

  # Test scalability with multiple elements
  for ncpu in 1 2 4 8
    do
    for u in 0 10 100
      do
      for elem in 0 1 5 10 25 50 100
	do
	./$i $duration ${u}/100 $elem $ncpu
      done > $dir/$i.cpu${ncpu}.u${u}
    done
  done

done

for i in test_hash_spinlock.exe \
    test_hash_lf_smr.exe test_hash_lf_qsbr.exe \
    test_hash_lf_ebr.exe test_hash_lf_nebr.exe \
    test_hash_cr_smr.exe test_hash_cr_qsbr.exe \
    test_hash_cr_ebr.exe test_hash_cr_nebr.exe
do
  echo "Testing $i"

  # Test scalability with multiple threads
  for elem in 1
    do
    for u in 0 10 100
      do
      for ncpu in 1 2 4 8 16 24 32
        do
        ./$i $duration $u/100 $((${elem}*${nbuckets})) $nbuckets $ncpu
      done > $dir/$i.lf${elem}.u${u}
    done
  done

  # Test scalability with respect to update fraction
  for ncpu in 1 2 4 8 16 32
    do
    for elem in 1
    do
      for u in 0 1 2 5 10 25 50 75 100
        do
        ./$i $duration $u/100 $((${elem}*${nbuckets})) $nbuckets $ncpu
      done > $dir/$i.cpu${ncpu}.elem${elem}
    done
  done

  # Test scalability respect to hash size (measures contention).
  for ncpu in 1 2 4 8 16 32
    do
    for lf in 1
      do
      for u in 0 10 100
        do
        for hs in 1 4 8 16 32
          do
          ./$i $duration $u/100 $((${lf}*${hs})) $hs $ncpu
        done > $dir/$i.cpu${ncpu}.lf${lf}.u${u}
      done
    done
  done

done

for i in test_queue_spinlock.exe \
    test_queue_lf_qsbr.exe test_queue_lf_ebr.exe \
    test_queue_lf_smr.exe test_queue_lf_nebr.exe
do
  echo "Testing $i"

  # Graph with respect to number of threads.
  for ncpu in 1 2 4 8 16 24 32
    do
    ./$i $duration $queuesize $ncpu
  done > $dir/$i.nq${nq}

done
