#!/bin/zsh -e

cheap=1
if [[ $cheap == 0 ]] ; then
    gcs=(ebr rc hp dw jvm)
    threads=(1 2 4 8 16 31 32 63 64) # 9
    elements=(100 200 400 800 1600) # 5
    iteration=($(seq 1 5))
    # unit: ms
    runtime=10000
else
    # gcs=(ebr rc hp dw jvm)
    gcs=(jvm)
    threads=(64) # 9
    elements=(1600) # 5
    # unit: ms
    runtime=10000
    iteration=($(seq 1 5))
fi
make clean
rm -f core
ulimit -c unlimited

typeset -A cores
cores[1]="0"
cores[2]="0,8"
cores[4]="0,8,16,24"
cores[8]="0,8,16,24,32,40,48,60"
cores[16]="0,8,16,24,32,40,48,60,2,10,18,26,34,42,50,58"
cores[31]="0,8,16,24,32,40,48,60,2,10,18,26,34,42,50,58,3,11,19,27,35,43,51,59,1,9,17,25,33,41,49"
cores[32]="0,8,16,24,32,40,48,60,2,10,18,26,34,42,50,58,3,11,19,27,35,43,51,59,1,9,17,25,33,41,49,57"
cores[63]="0-62"
cores[64]="0-63"

cat > db_prg.m << EOF
% all mr algo
ebr=1; rc=2; hp=3; dw=4; jvm=5;
gcs = [$gcs];
threads = [$threads];
elements = [$elements];
db_prg_total_avg = 0;
db_prg_total_std = 0;
for gc = gcs
    for t = threads
        for e = elements
            ans = sscanf(fgetl(0), "%u")';
            db_prg_total_avg(...
                find(gcs == gc), ...
                find(threads == t), ...
                find(elements == e) ...
                ) = ans(1);
            db_prg_total_std(...
                find(gcs == gc), ...
                find(threads == t), ...
                find(elements == e) ...
                ) = ans(2);
        endfor
    endfor
endfor

db_prg_footprint_avg = 0;
db_prg_footprint_std = 0;
for gc = gcs
    for t = threads
        for e = elements
            ans = sscanf(fgetl(0), "%u")';
            db_prg_footprint_avg(...
                find(gcs == gc), ...
                find(threads == t), ...
                find(elements == e) ...
                ) = ans(1);
            db_prg_footprint_std(...
                find(gcs == gc), ...
                find(threads == t), ...
                find(elements == e) ...
                ) = ans(2);
        endfor
    endfor
endfor
% db_prg_total_avg
% db_prg_total_std
% db_prg_footprint_avg
% db_prg_footprint_std
% save db_prg.mat db_prg_total_avg db_prg_total_std db_prg_footprint_avg db_prg_footprint_std
EOF
perl -pe 's!prg!queue!g' db_prg.m > db_queue.m

: > queue.total.log
: > queue.footprint.log
for gc in $gcs; do
    make -s test_queue_lf_${gc}
    for t in $threads; do
        for e in $elements; do
            echo "total = [" > total.m
            echo "footprint = [" > footprint.m
            for i in $iteration; do
                if [[ $gc == "dw" ]]; then
                    /usr/bin/time -f "%M" -o mem.txt \
                        ./test_queue_lf_${gc}.exe --ponythreads $(($t+1)) \
                        $runtime $e $t >> total.m
                elif [[ $gc == "jvm" ]]; then
                    /usr/bin/time -f "%M" -o mem.txt \
                        java MyQueue $runtime $e $t >> total.m
                else
                    /usr/bin/time -f "%M" -o mem.txt \
                        ./test_queue_lf_${gc}.exe $runtime $e $t >> total.m
                fi
                cat mem.txt >> footprint.m
            done
            rm -f mem.txt
            cat >> total.m << EOF
];
printf("%u %u\n", uint32(mean(total)), uint32(std(total)));
EOF
            cat >> footprint.m << EOF
];
printf("%u %u\n", uint32(mean(footprint)), uint32(std(footprint)));
EOF
            octave -q total.m >> queue.total.log
            octave -q footprint.m >> queue.footprint.log
            rm -f total.m
            rm -f footprint.m
        done
    done
done
cat queue.total.log queue.footprint.log | octave -q db_queue.m

exit
