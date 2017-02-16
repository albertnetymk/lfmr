#!/bin/zsh -e

cheap=0
if [[ $cheap == 0 ]] ; then
    gcs=(ebr rc hp dw)
    threads=(1 2 4 8 16 31 32 63 64) # 9
    elements=(100 200 400 800 1600) # 5
    update=(0 20 40 60 80 100) # 6
    iteration=($(seq 1 5))
    # unit: ms
    runtime=10000
else
    gcs=(ebr rc hp dw)
    threads=(2)
    elements=(100)
    update=(20)
    # unit: ms
    runtime=1000
    iteration=($(seq 1 1))
fi

cat > db_prg.m << EOF
% all mr algo
ebr=1; rc=2; hp=3; dw=4;
gcs = [$gcs];
threads = [$threads];
elements = [$elements];
update = [$update];
db_prg_total_avg = 0;
db_prg_total_std = 0;
for gc = gcs
    for t = threads
        for e = elements
            for u = update
                ans = sscanf(fgetl(0), "%u")';
                db_prg_total_avg(...
                    find(gcs == gc), ...
                    find(threads == t), ...
                    find(elements == e), ...
                    find(update == u) ...
                    ) = ans(1);
                db_prg_total_std(...
                    find(gcs == gc), ...
                    find(threads == t), ...
                    find(elements == e), ...
                    find(update == u) ...
                    ) = ans(2);
            endfor
        endfor
    endfor
endfor

db_prg_footprint_avg = 0;
db_prg_footprint_std = 0;
for gc = gcs
    for t = threads
        for e = elements
            for u = update
                ans = sscanf(fgetl(0), "%u")';
                db_prg_footprint_avg(...
                    find(gcs == gc), ...
                    find(threads == t), ...
                    find(elements == e), ...
                    find(update == u) ...
                    ) = ans(1);
                db_prg_footprint_std(...
                    find(gcs == gc), ...
                    find(threads == t), ...
                    find(elements == e), ...
                    find(update == u) ...
                    ) = ans(2);
            endfor
        endfor
    endfor
endfor
 db_prg_total_avg
 db_prg_total_std
 db_prg_footprint_avg
 db_prg_footprint_std
save db_prg.mat db_prg_total_avg db_prg_total_std db_prg_footprint_avg db_prg_footprint_std
EOF
perl -pe 's!prg!list!g' db_prg.m > db_list.m

: > list.total.log
: > list.footprint.log
for gc in $gcs; do
    make -s test_list_lf_${gc}
    for t in $threads; do
        for e in $elements; do
            for u in $update; do
                echo "total = [" > total.m
                echo "footprint = [" > footprint.m
                for i in $iteration; do
                    if [[ $gc == "dw" ]]; then
                        /usr/bin/time -f "%M" -o mem.txt \
                            ./test_list_lf_${gc}.exe --ponythreads $(($t+1)) \
                            $runtime $u $e $t >> total.m
                    else
                        /usr/bin/time -f "%M" -o mem.txt \
                        ./test_list_lf_${gc}.exe $runtime $u $e $t >> total.m
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
                octave -q total.m >> list.total.log
                octave -q footprint.m >> list.footprint.log
                rm -f total.m
                rm -f footprint.m
            done
        done
    done
done
cat list.total.log list.footprint.log | octave -q db_list.m

exit
