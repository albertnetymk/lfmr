# CFLAGS = -DUSE_BACKOFF -DYIELD -DUSE_AFFINITY
CFLAGS = -DYIELD -DUSE_AFFINITY
ALLOCATOR = allocator_malloc.c
CC=gcc -std=gnu11 -O3 -Wno-implicit-function-declaration -Wno-pointer-to-int-cast
# CC=clang -std=gnu11 -O3 -Wno-implicit-function-declaration
HELPERS = util.c main.c random.c

# default: test_queue_lf_ebr
default: test_queue_lf_ebr test_queue_lf_rc test_queue_lf_hp test_queue_lf_dw

.PHONY: clean
clean:
	rm -f *.o *.exe
	rm -f *.gif *.pbm *.eps *u0.sh *u10.sh *u100.sh *nq.sh *elem*.sh
	rm -rf encore_code/*_src

test_list_lf_ebr: test_list.c list_lf_ebr.c ebr.c $(HELPERS) $(ALLOCATOR)
	$(CC) $(CFLAGS) -o test_list_lf_ebr.exe $^ -lm -pthread

test_list_lf_rc: test_list.c list_lf_lfrc.c lfrc.c $(HELPERS)
	$(CC) $(CFLAGS) -o test_list_lf_rc.exe $^ -lm -pthread

test_list_lf_hp: test_list.c list_lf_smr.c smr.c $(HELPERS) $(ALLOCATOR)
	$(CC) $(CFLAGS) -o test_list_lf_hp.exe $^ -lm -pthread

test_list_lf_dw: ./encore_code/lf_list.enc
	encorec -O3 -c $< -o test_list_lf_dw.exe

test_queue_lf_ebr: test_queue.c queue_lf_ebr.c ebr.c $(HELPERS) $(ALLOCATOR)
	$(CC) $(CFLAGS) -o test_queue_lf_ebr.exe $^ -lm -pthread

test_queue_lf_rc: test_queue.c queue_lf_lfrc.c lfrc.c $(HELPERS)
	$(CC) $(CFLAGS) -o test_queue_lf_rc.exe $^ -lm -pthread

test_queue_lf_hp: test_queue.c queue_lf_smr.c smr.c $(HELPERS) $(ALLOCATOR)
	$(CC) $(CFLAGS) -o test_queue_lf_hp.exe $^ -lm -pthread

test_queue_lf_dw: ./encore_code/lf_queue.enc
	encorec -O3 -c $< -o test_queue_lf_dw.exe
