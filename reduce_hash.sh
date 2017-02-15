#!/bin/sh
# One invocation creates a .dat file based on standard input

if test "$1" = -update
then
	selector='$7/$8'
fi
if test "$1" = -nthreads
then
	selector='$14'
fi
if test "$1" = -lf
then
	selector='$10/$12'
fi
if test "$1" = -hashsize
then
	selector='$12'
fi
measured="avg"

sed -e 's,/, ,g' | awk '
	{
	if ($3 == "nmilli:") {
		outline = '"$selector"'
		cpus = $14
	}
	if (($3 == "avg") && ($3 == "'"$measured"'")) {
		print outline, cpus*$5, cpus*$8, cpus*$11
	}
	if (($4 == "avg") && ($3 == "'"$measured"'")) {
		print outline, $6, $9, $12
	}
	}'
