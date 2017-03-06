# CFLAGS = -DUSE_BACKOFF -DYIELD -DUSE_AFFINITY
CFLAGS = -DNDEBUG -DYIELD -DUSE_AFFINITY
# CFLAGS = -DYIELD -DUSE_AFFINITY
ALLOCATOR = allocator_malloc.c
CC=gcc -std=gnu11 -O3 -Wno-implicit-function-declaration -Wno-pointer-to-int-cast
# CC=gcc -g -pg -O3 -std=gnu11 -Wno-implicit-function-declaration -Wno-pointer-to-int-cast
# CC=clang -g -pg -std=gnu11 -O3 -Wno-implicit-function-declaration
HELPERS = util.c main.c random.c

# default: test_queue_lf_ebr
default: test_queue_lf_ebr test_queue_lf_rc test_queue_lf_hp test_queue_lf_dw

.PHONY: clean
clean:
	rm -f *.class
	rm -f *.o *.exe
	rm -f *.gif *.pbm *.eps *u0.sh *u10.sh *u100.sh *nq.sh *elem*.sh
	rm -rf encore_code/*_src

.PHONY: mk_dir
mk_dir:
	mkdir -p plots/list/exec_time/
	mkdir -p plots/list/footprint/
	mkdir -p plots/list/include_rc_exec_time/
	mkdir -p plots/list/include_rc_footprint/

	mkdir -p plots/queue/exec_time/
	mkdir -p plots/queue/footprint/

test_list_lf_ebr: test_list.c list_lf_ebr.c ebr.c $(HELPERS) $(ALLOCATOR)
	$(CC) $(CFLAGS) -o test_list_lf_ebr.exe $^ -lm -pthread

test_list_lf_rc: test_list.c list_lf_lfrc.c lfrc.c $(HELPERS)
	$(CC) $(CFLAGS) -o test_list_lf_rc.exe $^ -lm -pthread

test_list_lf_hp: test_list.c list_lf_smr.c smr.c $(HELPERS) $(ALLOCATOR)
	$(CC) $(CFLAGS) -o test_list_lf_hp.exe $^ -lm -pthread

test_list_lf_dw: ./encore_code/lf_list.enc
	encorec -O3 -c $< -o test_list_lf_dw.exe
	# encorec -c $< -o test_list_lf_dw.exe

test_list_lf_jvm: MyList.java
	javac $<

# test_list_lf_dw: test_list.c list_lf_dw.c dw.c $(HELPERS) $(ALLOCATOR)
#     $(CC) $(CFLAGS) -o test_list_lf_dw.exe $^ -lm -pthread

# test_list_lf_dw: test_list.c list_lf_dw.c auto_ebr.c $(HELPERS) $(ALLOCATOR)
#     $(CC) $(CFLAGS) -o test_list_lf_dw.exe $^ -lm -pthread

test_queue_lf_ebr: test_queue.c queue_lf_ebr.c ebr.c $(HELPERS) $(ALLOCATOR)
	$(CC) $(CFLAGS) -o test_queue_lf_ebr.exe $^ -lm -pthread

test_queue_lf_rc: test_queue.c queue_lf_lfrc.c lfrc.c $(HELPERS)
	$(CC) $(CFLAGS) -o test_queue_lf_rc.exe $^ -lm -pthread

test_queue_lf_hp: test_queue.c queue_lf_smr.c smr.c $(HELPERS) $(ALLOCATOR)
	$(CC) $(CFLAGS) -o test_queue_lf_hp.exe $^ -lm -pthread

test_queue_lf_dw: ./encore_code/lf_queue.enc
	encorec -O3 -c $< -o test_queue_lf_dw.exe

# test_queue_lf_dw: test_queue.c queue_lf_dw.c dw.c $(HELPERS) $(ALLOCATOR)
#     $(CC) $(CFLAGS) -o test_queue_lf_dw.exe $^ -lm -pthread

test_queue_lf_jvm: MyQueue.java
	javac $<

