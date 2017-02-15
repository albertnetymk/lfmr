#!/bin/sh
# One invocation creates a .dat file based on standard input

if test "$1" = -nthreads
then
	selector='$8'
fi
measured="avg"

sed -e 's,/, ,g' | awk '
	{
	if ($3 == "nmilli:") {
		outline = '"$selector"'
		cpus = $8
	}
	if (($3 == "avg") && ($3 == "'"$measured"'")) {
		print outline, cpus*$5, cpus*$8, cpus*$11
	}
	if (($4 == "avg") && ($3 == "'"$measured"'")) {
		print outline, $6, $9, $12
	}
	}'
