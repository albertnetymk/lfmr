#!/bin/sh
# This file just creates the .dat files by calling reduce.sh

# Identify data files

lfrcs="test_list_lf_lfrc.exe test_list_cr_lfrc.exe"
lists="test_list_spinlock.exe test_list_lf_smr.exe test_list_lf_qsbr.exe test_list_lf_ebr.exe test_list_lf_nebr.exe test_list_cr_smr.exe test_list_cr_qsbr.exe test_list_cr_ebr.exe test_list_cr_nebr.exe"
hashes="test_hash_spinlock.exe test_hash_lf_smr.exe test_hash_lf_qsbr.exe test_hash_lf_ebr.exe test_hash_lf_nebr.exe test_hash_cr_smr.exe test_hash_cr_qsbr.exe test_hash_cr_ebr.exe test_hash_cr_nebr.exe"
queues="test_queue_spinlock.exe test_queue_lf_qsbr.exe test_queue_lf_ebr.exe test_queue_lf_smr.exe test_queue_lf_nebr.exe"

ncpusuffixes="cpu1 cpu2 cpu4 cpu8 cpu16 cpu32"
elemsuffixes="elem1 elem10 elem50 elem100"
elemsuffixes2="elem1 elem5 elem10 elem20 elem30 elem50 elem100"
lfsuffixes="lf1"
helemsuffixes="elem1"
updatesuffixes="u0 u10 u100"

# LFRC
for i in $lfrcs
do
  for a in $ncpusuffixes
  do
    for b in "u0"
    do
      sh ../reduce_list.sh -nelements < $i.$a.$b > $i.$a.$b.dat
    done
  done
done

# Linked lists
for i in $lists
do
  for a in $ncpusuffixes
  do
    for b in $elemsuffixes
    do
      sh ../reduce_list.sh -update < $i.$a.$b > $i.$a.$b.dat
    done
  done

  for a in $elemsuffixes2
  do
    for b in $updatesuffixes
    do
      sh ../reduce_list.sh -nthreads < $i.$a.$b > $i.$a.$b.dat
    done
  done

  for a in $ncpusuffixes
  do
    for b in $updatesuffixes
    do
      sh ../reduce_list.sh -nelements < $i.$a.$b > $i.$a.$b.dat
    done
  done
done

# Hash tables
for i in $hashes
do
	# Create the table for update fractions
        for a in $ncpusuffixes
	do
	  for b in $helemsuffixes
	  do
	    sh ../reduce_hash.sh -update < $i.$a.$b > $i.$a.$b.dat
	  done
	done

        # Create the table for hash sizes
	for a in $ncpusuffixes
	do
	  for b in $lfsuffixes
	  do
	    for c in $updatesuffixes
	    do
		sh ../reduce_hash.sh -hashsize < $i.$a.$b.$c > $i.$a.$b.$c.dat
	    done
	  done
	done

	# Create the table for number of threads
	for a in $lfsuffixes
	do
	  for b in $updatesuffixes
	  do
	    sh ../reduce_hash.sh -nthreads < $i.$a.$b > $i.$a.$b.dat
	  done
	done
done

# Queues
for i in $queues
do
  sh ../reduce_queue.sh -nthreads < $i.nq > $i.nq.dat
done