#!/bin/sh

sh ../reducegraphtests.sh

fontsize=10
plotsize=0.5
plotsize_y=0.65

lflists="test_list_lf_smr.exe test_list_lf_qsbr.exe test_list_lf_ebr.exe"
lflists_nebr="test_list_lf_smr.exe test_list_lf_qsbr.exe test_list_lf_ebr.exe test_list_lf_nebr.exe"
lflists_lock="test_list_lf_smr.exe test_list_lf_qsbr.exe test_list_lf_ebr.exe test_list_spinlock.exe"
lflists_lfrc="test_list_lf_smr.exe test_list_lf_qsbr.exe test_list_lf_ebr.exe test_list_lf_lfrc.exe"
lfhashes="test_hash_lf_smr.exe test_hash_lf_qsbr.exe test_hash_lf_ebr.exe"
lfhashes_lock="test_hash_lf_smr.exe test_hash_lf_qsbr.exe test_hash_lf_ebr.exe test_hash_spinlock.exe"
lfhashes_nebr="test_hash_lf_smr.exe test_hash_lf_qsbr.exe test_hash_lf_ebr.exe test_hash_lf_nebr.exe"
lfcrhashes="test_hash_lf_smr.exe test_hash_cr_smr.exe test_hash_lf_qsbr.exe test_hash_cr_qsbr.exe"
queues="test_queue_lf_smr.exe test_queue_lf_qsbr.exe test_queue_lf_ebr.exe test_queue_lf_nebr.exe test_queue_spinlock.exe"
lfqueues="test_queue_lf_smr.exe test_queue_lf_qsbr.exe test_queue_lf_ebr.exe"
lfqueues_nebr="test_queue_lf_smr.exe test_queue_lf_qsbr.exe test_queue_lf_ebr.exe test_queue_lf_nebr.exe"
function plotfile () {
# Original:
#	echo '"'$1'" w linespoints, "'$1'" notitle w yerrorbars'
# Cleaner, and with error bars
#	echo '"'$1'" with yerrorlines pointsize 2'
# Cleaner, no error bars (easier to read)
	echo '"'$1'" with linespoints pointsize 1'
	}

function makeplotline () {
	local firsttime suffix plotline i

	firsttime="yes"
	suffix="$2"
	plotline=""
	set $1
	for i in $*
	do
		if test "$firsttime" = "no"
		then
			plotline="${plotline}, "
		fi
		firsttime="no"
		plotline="${plotline}`plotfile $i.${suffix}.dat`"
	done
	echo ${plotline}
	}

function gnuplotit () {
	gnuplot << ---EOF---
		set term pbm medium
		set size 2,2
		set output "$1.pbm"
		set xlabel "$2"
		set ylabel "Avg CPU Time (ns)"
		$3
		plot $4
		set term postscript eps ${fontsize}
		set size square ${plotsize},${plotsize_y}
		set output "$1.eps"
		replot
---EOF---
	ppmtogif $1.pbm > $1.gif 2> /dev/null

	# Also generate a script so we can make nice EPS output. :)
	touch $1.sh
	rm $1.sh
	echo  "gnuplot << ---EOF---" >> $1.sh
        echo  "set xlabel \"$2\"" >> $1.sh
	echo  "set ylabel \"Avg CPU Time (ns)" >> $1.sh
	echo  "$3" >> $1.sh
        echo  "set term postscript eps ${fontsize}" >> $1.sh
	echo  "set size square ${plotsize},${plotsize_y}" >> $1.sh
	echo  "set output \"$1.eps\"" >> $1.sh
	echo  "plot $4" >> $1.sh
        echo  "---EOF---"  >> $1.sh
}

function plotsuffixes_list () {
	local s
	local xlabel

	xlabel="$1"
	set $2
	for s in $*
	do

		plotline=`makeplotline "$lflists" $s`
		gnuplotit lflists.$s "$xlabel" "set key left top Left" "${plotline}"

		plotline=`makeplotline "$lflists_nebr" $s`
		gnuplotit lflists_nebr.$s "$xlabel" "set key left top Left" "${plotline}"

		plotline=`makeplotline "$lflists_lock" $s`
		gnuplotit lflists_lock.$s "$xlabel" "set key left top Left" "${plotline}"

	done
}

function plotsuffixes_list_lfrc () {
	local s
	local xlabel

	xlabel="$1"
	set $2
	for s in $*
	do

		plotline=`makeplotline "$lflists_lfrc" $s`
		gnuplotit lflists_lfrc.$s "$xlabel" "set key left top Left" "${plotline}"
	done
}

function plotsuffixes_hash () {
	local s
	local xlabel

	xlabel="$1"
	set $2
	for s in $*
	do

		plotline=`makeplotline "$lfhashes" $s`
		gnuplotit lfhashes.$s "$xlabel" "set key left top Left" "${plotline}"

		plotline=`makeplotline "$lfcrhashes" $s`
		gnuplotit lfcrhashes.$s "$xlabel" "set key left top Left" "${plotline}"

		plotline=`makeplotline "$lfhashes_nebr" $s`
		gnuplotit lfhashes_nebr.$s "$xlabel" "set key left top Left" "${plotline}"

		plotline=`makeplotline "$lfhashes_lock" $s`
		gnuplotit lfhashes_lock.$s "$xlabel" "set key left top Left" "${plotline}"

	done
}

function plotsuffixes_queue () {
	local s
	local xlabel

	xlabel="$1"
	set $2
	for s in $*
	do

		plotline=`makeplotline "$queues" $s`
		gnuplotit queues.$s "$xlabel" "set key left top Left" "${plotline}"

		plotline=`makeplotline "$lfqueues" $s`
		gnuplotit lfqueues.$s "$xlabel" "set key left top Left" "${plotline}"

		plotline=`makeplotline "$lfqueues_nebr" $s`
		gnuplotit lfqueues_nebr.$s "$xlabel" "set key left top Left" "${plotline}"

	done
}

plotsuffixes_list "Number of Threads" "elem1.u0 elem1.u10 elem1.u100 elem5.u0 elem5.u10 elem5.u100 elem10.u0 elem10.u10 elem10.u100 elem20.u0 elem20.u10 elem20.u100 elem30.u0 elem30.u10 elem30.u100 elem50.u0 elem50.u10 elem50.u100 elem100.u0 elem100.u10 elem100.u100"

plotsuffixes_list "Number of Elements" "cpu1.u0 cpu1.u10 cpu1.u100 cpu2.u0 cpu2.u10 cpu2.u100 cpu4.u0 cpu4.u10 cpu4.u100 cpu8.u0 cpu8.u10 cpu8.u100 cpu16.u0 cpu16.u10 cpu16.u100 cpu32.u0 cpu32.u10 cpu32.u100"

plotsuffixes_list "Update Fraction" "cpu1.elem1 cpu1.elem10 cpu1.elem50 cpu1.elem100 cpu2.elem1 cpu2.elem10 cpu2.elem50 cpu2.elem100 cpu4.elem1 cpu4.elem10 cpu4.elem50 cpu4.elem100 cpu8.elem1 cpu8.elem10 cpu8.elem50 cpu8.elem100 cpu16.elem1 cpu16.elem10 cpu16.elem50 cpu16.elem100 cpu32.elem1 cpu32.elem10 cpu32.elem50 cpu32.elem100"

plotsuffixes_list_lfrc "Number of Elements" "cpu1.u0 cpu2.u0 cpu4.u0 cpu8.u0 cpu16.u0 cpu32.u0"

plotsuffixes_hash "Number of Threads" "lf1.u0 lf1.u10 lf1.u100"

plotsuffixes_hash "Update Fraction" "cpu1.elem1 cpu2.elem1 cpu4.elem1 cpu8.elem1 cpu16.elem1 cpu32.elem1"

plotsuffixes_hash "Number of Buckets" "cpu1.lf1.u0 cpu1.lf1.u10 cpu1.lf1.u100 cpu2.lf1.u0 cpu2.lf1.u10 cpu2.lf1.u100 cpu4.lf1.u0 cpu4.lf1.u10 cpu4.lf1.u100 cpu8.lf1.u0 cpu8.lf1.u10 cpu8.lf1.u100 cpu16.lf1.u0 cpu16.lf1.u10 cpu16.lf1.u100 cpu32.lf1.u0 cpu32.lf1.u10 cpu32.lf1.u100"



plotsuffixes_queue "Number of Threads" "nq"
