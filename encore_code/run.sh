#!/usr/bin/zsh
set -e
rm -f core
ulimit -c unlimited
files=()

files+=(lf_list.enc)
# files+=(lf_queue.enc)

gcs=(dw)
threads=(1 2 4 8 16 31 32 63 64) # 9
# threads=(4) # 9
elements=(100 200 400 800 1600) # 5
update=(0 20 40 60 80 100) # 6
iteration=($(seq 1 1))
runtime=1000

files=()
files+=(lf_list.enc)
files+=(lf_queue.enc)

for file in $files; do
    # encorec -c -g $file
    encorec -c -O3 -g $file

    if [[ $file == "lf_list.enc" ]]; then
        for t in $threads; do
            for e in $elements; do
                for u in $update; do
                    for i in $iteration; do
                        echo "./lf_list $runtime $u $e $t"
                        ./lf_list --ponythreads $(($t+1)) $runtime $u $e $t > /dev/null
                        ret=$?
                        if [ $ret -ne 0 ]; then
                            echo "error"
                            exit $ret
                        fi
                    done
                done
            done
        done
    fi

    if [[ $file == "lf_queue.enc" ]]; then
        for t in $threads; do
            for e in $elements; do
                for i in $iteration; do
                    echo "./lf_queue $runtime $e $t"
                    ./lf_queue --ponythreads $(($t+1)) $runtime $e $t > /dev/null
                    ret=$?
                    if [ $ret -ne 0 ]; then
                        echo "error"
                        exit $ret
                    fi
                done
            done
        done
    fi
done
