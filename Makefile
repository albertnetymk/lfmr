#############################################################################
#
# Definitions
#
#############################################################################

CFLAGS = -g -lm -DUSE_BACKOFF -DYIELD
CFLAGS_LIB = -g -DUSE_BACKOFF -DYIELD
ALLOCATOR = allocator_magazine.o

#############################################################################
#
# Architectural stuff
#
#############################################################################

x86:
	cp arch/x86/*.h arch/

ppc:
	cp arch/ppc32/*.h arch/

#############################################################################
#
# Main targets
#
#############################################################################

clean:
	touch nofail.o
	touch nofail.exe
	rm *.o *.exe

all: test_list_null test_list_spinlock test_list_mcslock \
test_list_lf_smr test_list_lf_qsbr test_list_lf_ebr test_list_lf_nebr \
test_list_lf_lfrc test_list_lf_hybrid_qsbr test_list_lf_hybrid_nebr \
test_list_cr_smr test_list_cr_qsbr test_list_cr_ebr test_list_cr_nebr \
test_list_cr_lfrc \
test_hash_spinlock \
test_hash_lf_smr test_hash_lf_qsbr test_hash_lf_ebr test_hash_lf_nebr \
test_hash_lf_lfrc \
test_hash_cr_smr test_hash_cr_qsbr test_hash_cr_ebr test_hash_cr_nebr \
test_hash_cr_lfrc \
test_queue_spinlock \
test_queue_lf_qsbr test_queue_lf_ebr test_queue_lf_smr test_queue_lf_nebr \
test_queue_lf_lfrc

#############################################################################
#
# Utility stuff
#
#############################################################################

random.o: random.h random.c test.h
	gcc $(CFLAGS_LIB) -c -o random.o random.c

util.o: util.h util.c
	gcc $(CFLAGS_LIB) -c -o util.o util.c

allocator_malloc.o: node.h allocator.h allocator_malloc.c
	gcc $(CFLAGS_LIB) -c -o allocator_malloc.o allocator_malloc.c

allocator_magazine.o: node.h allocator.h allocator_magazine.c
	gcc $(CFLAGS_LIB) -c -o allocator_magazine.o allocator_magazine.c

backoff.o: backoff.h backoff.c test.h
	gcc $(CFLAGS_LIB) -c -o backoff.o backoff.c

#############################################################################
#
# Test "driver" code
#
#############################################################################

main_test_default.o: test.h main_test_default.c
	gcc $(CFLAGS_LIB) -c -o main_test_default.o main_test_default.c

main_test_qsbr.o: test.h main_test_qsbr.c
	gcc $(CFLAGS_LIB) -c -o main_test_qsbr.o main_test_qsbr.c

main_test_nebr.o: test.h main_test_nebr.c
	gcc $(CFLAGS_LIB) -c -o main_test_nebr.o main_test_nebr.c

test_list.o: test.h list.h test_list.c mr.h
	gcc $(CFLAGS_LIB) -c -o test_list.o test_list.c

test_hash.o: test.h hash.h test_hash.c mr.h
	gcc $(CFLAGS_LIB) -c -o test_hash.o test_hash.c

test_queue.o: test.h queue.h test_queue.c mr.h
	gcc $(CFLAGS_LIB) -c -o test_queue.o test_queue.c

#############################################################################
#
# Implementations of MR schemes
#
#############################################################################

lbr.o: mr.h lbr.c
	gcc $(CFLAGS_LIB) -c -o lbr.o lbr.c

smr.o: smr.h smr.c test.h node.h mr.h
	gcc $(CFLAGS_LIB) -c -o smr.o smr.c

qsbr.o: qsbr.h qsbr.c test.h node.h mr.h
	gcc $(CFLAGS_LIB) -c -o qsbr.o qsbr.c

ebr.o: ebr.h ebr.c test.h node.h mr.h
	gcc $(CFLAGS_LIB) -c -o ebr.o ebr.c

nebr.o: nebr.h nebr.c test.h node.h mr.h
	gcc $(CFLAGS_LIB) -c -o nebr.o nebr.c

lfrc.o: lfrc.h lfrc.c test.h node.h arch/atomic.h allocator.h
	gcc $(CFLAGS_LIB) -c -o lfrc.o lfrc.c

#############################################################################
#
# Implementations of specific list/MR scheme pairings.
#
#############################################################################

list_null.o: list.h node.h spinlock.h list_null.c
	gcc $(CFLAGS_LIB) -c -o list_null.o list_null.c

list_spinlock.o: list.h node.h spinlock.h list_spinlock.c
	gcc $(CFLAGS_LIB) -c -o list_spinlock.o list_spinlock.c

list_mcslock.o: list.h node.h mcslock.h list_mcslock.c
	gcc $(CFLAGS_LIB) -c -o list_mcslock.o list_mcslock.c

list_lf_smr.o: list.h node.h test.h smr.h smr.c list_lf_smr.c
	gcc $(CFLAGS_LIB) -c -o list_lf_smr.o list_lf_smr.c

list_lf_qsbr.o: list.h node.h test.h qsbr.h qsbr.c list_lf_qsbr.c
	gcc $(CFLAGS_LIB) -c -o list_lf_qsbr.o list_lf_qsbr.c

list_lf_hybrid_qsbr.o: list.h node.h test.h qsbr.h qsbr.c list_lf_hybrid_qsbr.c
	gcc $(CFLAGS_LIB) -c -o list_lf_hybrid_qsbr.o list_lf_hybrid_qsbr.c

list_lf_ebr.o: list.h node.h test.h ebr.h ebr.c list_lf_ebr.c
	gcc $(CFLAGS_LIB) -c -o list_lf_ebr.o list_lf_ebr.c

list_lf_nebr.o: list.h node.h test.h nebr.h nebr.c list_lf_nebr.c
	gcc $(CFLAGS_LIB) -c -o list_lf_nebr.o list_lf_nebr.c

list_lf_hybrid_nebr.o: list.h node.h test.h nebr.h nebr.c list_lf_hybrid_nebr.c
	gcc $(CFLAGS_LIB) -c -o list_lf_hybrid_nebr.o list_lf_hybrid_nebr.c

list_lf_lfrc.o: list.h node.h test.h lfrc.h lfrc.c list_lf_lfrc.c
	gcc $(CFLAGS_LIB) -c -o list_lf_lfrc.o list_lf_lfrc.c

list_cr_smr.o: list.h node.h test.h smr.h smr.c list_cr_smr.c
	gcc $(CFLAGS_LIB) -c -o list_cr_smr.o list_cr_smr.c

list_cr_qsbr.o: list.h node.h test.h qsbr.h qsbr.c list_cr_qsbr.c
	gcc $(CFLAGS_LIB) -c -o list_cr_qsbr.o list_cr_qsbr.c

list_cr_ebr.o: list.h node.h test.h ebr.h ebr.c list_cr_ebr.c
	gcc $(CFLAGS_LIB) -c -o list_cr_ebr.o list_cr_ebr.c

list_cr_nebr.o: list.h node.h test.h nebr.h nebr.c list_cr_nebr.c
	gcc $(CFLAGS_LIB) -c -o list_cr_nebr.o list_cr_nebr.c

list_cr_lfrc.o: list.h node.h test.h lfrc.h lfrc.c list_cr_lfrc.c
	gcc $(CFLAGS_LIB) -c -o list_cr_lfrc.o list_cr_lfrc.c

#############################################################################
#
# Hash table. Link with different list/MR scheme pairings.
#
#############################################################################

hash.o: hash.h node.h test.h hash.c
	gcc $(CFLAGS_LIB) -c -o hash.o hash.c

#############################################################################
#
# Implementations of specific queue/MR scheme pairings.
#
#############################################################################

queue_spinlock.o: queue.h node.h test.h spinlock.h queue_spinlock.c
	gcc $(CFLAGS_LIB) -c -o queue_spinlock.o queue_spinlock.c

queue_lf_qsbr.o: queue.h node.h test.h qsbr.h qsbr.c queue_lf_qsbr.c
	gcc $(CFLAGS_LIB) -c -o queue_lf_qsbr.o queue_lf_qsbr.c

queue_lf_ebr.o: queue.h node.h test.h ebr.h ebr.c queue_lf_ebr.c
	gcc $(CFLAGS_LIB) -c -o queue_lf_ebr.o queue_lf_ebr.c

queue_lf_nebr.o: queue.h node.h test.h nebr.h nebr.c queue_lf_nebr.c
	gcc $(CFLAGS_LIB) -c -o queue_lf_nebr.o queue_lf_nebr.c

queue_lf_smr.o: queue.h node.h test.h smr.h smr.c queue_lf_smr.c
	gcc $(CFLAGS_LIB) -c -o queue_lf_smr.o queue_lf_smr.c

queue_lf_lfrc.o: queue.h node.h test.h lfrc.h lfrc.c queue_lf_lfrc.c
	gcc $(CFLAGS_LIB) -c -o queue_lf_lfrc.o queue_lf_lfrc.c

#############################################################################
#
# Linking of executables.
#
#############################################################################

# Lists

test_list_null: test_list.o list_null.o util.o lbr.o main.c random.o main_test_default.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_list_null.exe main.c list_null.o lbr.o test_list.o util.o random.o main_test_default.o backoff.o $(ALLOCATOR)

test_list_spinlock: test_list.o list_spinlock.o util.o lbr.o main.c random.o main_test_default.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_list_spinlock.exe main.c list_spinlock.o lbr.o test_list.o util.o random.o main_test_default.o backoff.o $(ALLOCATOR)

test_list_mcslock: test_list.o list_mcslock.o util.o lbr.o main.c random.o main_test_default.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_list_mcslock.exe main.c list_mcslock.o lbr.o test_list.o util.o random.o main_test_default.o backoff.o $(ALLOCATOR)

test_list_lf_smr: test_list.o list_lf_smr.o util.o smr.o main.c random.o main_test_default.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_list_lf_smr.exe main.c list_lf_smr.o smr.o test_list.o util.o random.o main_test_default.o backoff.o $(ALLOCATOR)

test_list_lf_qsbr: test_list.o list_lf_qsbr.o util.o qsbr.o main.c random.o main_test_qsbr.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_list_lf_qsbr.exe main.c list_lf_qsbr.o qsbr.o test_list.o util.o random.o main_test_qsbr.o backoff.o $(ALLOCATOR)

test_list_lf_hybrid_qsbr: test_list.o list_lf_hybrid_qsbr.o util.o qsbr.o main.c random.o main_test_qsbr.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_list_lf_hybrid_qsbr.exe main.c list_lf_hybrid_qsbr.o qsbr.o test_list.o util.o random.o main_test_qsbr.o backoff.o $(ALLOCATOR)

test_list_lf_ebr: test_list.o list_lf_ebr.o util.o ebr.o main.c random.o main_test_default.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_list_lf_ebr.exe main.c list_lf_ebr.o ebr.o test_list.o util.o random.o main_test_default.o backoff.o $(ALLOCATOR)

test_list_lf_nebr: test_list.o list_lf_nebr.o util.o nebr.o main.c random.o main_test_nebr.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_list_lf_nebr.exe main.c list_lf_nebr.o nebr.o test_list.o util.o random.o main_test_nebr.o backoff.o $(ALLOCATOR)

test_list_lf_hybrid_nebr: test_list.o list_lf_hybrid_nebr.o util.o nebr.o main.c random.o main_test_nebr.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_list_lf_hybrid_nebr.exe main.c list_lf_hybrid_nebr.o nebr.o test_list.o util.o random.o main_test_nebr.o backoff.o $(ALLOCATOR)

test_list_lf_lfrc: test_list.o list_lf_lfrc.o util.o lfrc.o main.c random.o main_test_default.o backoff.o
	gcc $(CFLAGS) -o test_list_lf_lfrc.exe main.c list_lf_lfrc.o lfrc.o test_list.o util.o random.o main_test_default.o backoff.o

test_list_cr_smr: test_list.o list_cr_smr.o util.o smr.o main.c random.o main_test_default.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_list_cr_smr.exe main.c list_cr_smr.o smr.o test_list.o util.o random.o main_test_default.o backoff.o $(ALLOCATOR)

test_list_cr_qsbr: test_list.o list_cr_qsbr.o util.o qsbr.o main.c random.o main_test_qsbr.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_list_cr_qsbr.exe main.c list_cr_qsbr.o qsbr.o test_list.o util.o random.o main_test_qsbr.o backoff.o $(ALLOCATOR)

test_list_cr_ebr: test_list.o list_cr_ebr.o util.o ebr.o main.c random.o main_test_default.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_list_cr_ebr.exe main.c list_cr_ebr.o ebr.o test_list.o util.o random.o main_test_default.o backoff.o $(ALLOCATOR)

test_list_cr_nebr: test_list.o list_cr_nebr.o util.o nebr.o main.c random.o main_test_nebr.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_list_cr_nebr.exe main.c list_cr_nebr.o nebr.o test_list.o util.o random.o main_test_nebr.o backoff.o $(ALLOCATOR)

test_list_cr_lfrc: test_list.o list_cr_lfrc.o util.o lfrc.o main.c random.o main_test_default.o backoff.o
	gcc $(CFLAGS) -o test_list_cr_lfrc.exe main.c list_cr_lfrc.o lfrc.o test_list.o util.o random.o main_test_default.o backoff.o

# Hash tables

test_hash_spinlock: test_hash.o hash.o list_spinlock.o util.o lbr.o main.c random.o main_test_default.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_hash_spinlock.exe main.c hash.o list_spinlock.o lbr.o test_hash.o util.o random.o main_test_default.o backoff.o $(ALLOCATOR)

test_hash_lf_smr: test_hash.o hash.o list_lf_smr.o util.o smr.o main.c random.o main_test_default.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_hash_lf_smr.exe hash.o main.c list_lf_smr.o smr.o test_hash.o util.o random.o main_test_default.o backoff.o $(ALLOCATOR)

test_hash_lf_qsbr: test_hash.o hash.o list_lf_qsbr.o util.o qsbr.o main.c random.o main_test_qsbr.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_hash_lf_qsbr.exe main.c hash.o list_lf_qsbr.o qsbr.o test_hash.o util.o random.o main_test_qsbr.o backoff.o $(ALLOCATOR)

test_hash_lf_hybrid_qsbr: test_hash.o hash.o list_lf_hybrid_qsbr.o util.o qsbr.o main.c random.o main_test_qsbr.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_hash_lf_hybrid_qsbr.exe main.c hash.o list_lf_hybrid_qsbr.o qsbr.o test_hash.o util.o random.o main_test_qsbr.o backoff.o $(ALLOCATOR)

test_hash_lf_ebr: test_hash.o hash.o list_lf_ebr.o util.o ebr.o main.c random.o main_test_default.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_hash_lf_ebr.exe main.c hash.o list_lf_ebr.o ebr.o test_hash.o util.o random.o main_test_default.o backoff.o $(ALLOCATOR)

test_hash_lf_nebr: test_hash.o hash.o list_lf_nebr.o util.o nebr.o main.c random.o main_test_nebr.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_hash_lf_nebr.exe main.c hash.o list_lf_nebr.o nebr.o test_hash.o util.o random.o main_test_nebr.o backoff.o $(ALLOCATOR)

test_hash_lf_hybrid_nebr: test_hash.o hash.o list_lf_hybrid_nebr.o util.o nebr.o main.c random.o main_test_nebr.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_hash_lf_hybrid_nebr.exe main.c hash.o list_lf_hybrid_nebr.o nebr.o test_hash.o util.o random.o main_test_nebr.o backoff.o $(ALLOCATOR)

test_hash_lf_lfrc: test_hash.o hash.o list_lf_lfrc.o util.o lfrc.o main.c random.o main_test_default.o backoff.o
	gcc $(CFLAGS) -o test_hash_lf_lfrc.exe main.c hash.o list_lf_lfrc.o lfrc.o test_hash.o util.o random.o main_test_default.o backoff.o

test_hash_cr_smr: test_hash.o hash.o list_cr_smr.o util.o smr.o main.c random.o main_test_default.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_hash_cr_smr.exe main.c hash.o list_cr_smr.o smr.o test_hash.o util.o random.o main_test_default.o backoff.o $(ALLOCATOR)

test_hash_cr_qsbr: test_hash.o hash.o list_cr_qsbr.o util.o qsbr.o main.c random.o main_test_qsbr.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_hash_cr_qsbr.exe main.c hash.o list_cr_qsbr.o qsbr.o test_hash.o util.o random.o main_test_qsbr.o backoff.o $(ALLOCATOR)

test_hash_cr_ebr: test_hash.o hash.o list_cr_ebr.o util.o ebr.o main.c random.o main_test_default.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_hash_cr_ebr.exe main.c hash.o list_cr_ebr.o ebr.o test_hash.o util.o random.o main_test_default.o backoff.o $(ALLOCATOR)

test_hash_cr_nebr: test_hash.o hash.o list_cr_nebr.o util.o nebr.o main.c random.o main_test_nebr.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_hash_cr_nebr.exe main.c hash.o list_cr_nebr.o nebr.o test_hash.o util.o random.o main_test_nebr.o backoff.o $(ALLOCATOR)

test_hash_cr_lfrc: test_hash.o hash.o list_cr_lfrc.o util.o lfrc.o main.c random.o main_test_default.o backoff.o
	gcc $(CFLAGS) -o test_hash_cr_lfrc.exe main.c hash.o list_cr_lfrc.o lfrc.o test_hash.o util.o random.o main_test_default.o backoff.o

# Queues

test_queue_spinlock: test_queue.o queue_spinlock.o util.o main.c random.o main_test_default.o lbr.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_queue_spinlock.exe main.c queue_spinlock.o test_queue.o util.o random.o main_test_default.o lbr.o backoff.o $(ALLOCATOR)

test_queue_lf_qsbr: test_queue.o queue_lf_qsbr.o util.o qsbr.o main.c random.o main_test_qsbr.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_queue_lf_qsbr.exe main.c queue_lf_qsbr.o qsbr.o test_queue.o util.o random.o main_test_qsbr.o backoff.o $(ALLOCATOR)

test_queue_lf_ebr: test_queue.o queue_lf_ebr.o util.o ebr.o main.c random.o main_test_default.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_queue_lf_ebr.exe main.c queue_lf_ebr.o ebr.o test_queue.o util.o random.o main_test_default.o backoff.o $(ALLOCATOR)

test_queue_lf_nebr: test_queue.o queue_lf_nebr.o util.o nebr.o main.c random.o main_test_nebr.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_queue_lf_nebr.exe main.c queue_lf_nebr.o nebr.o test_queue.o util.o random.o main_test_nebr.o backoff.o $(ALLOCATOR)

test_queue_lf_smr: test_queue.o queue_lf_smr.o util.o smr.o main.c random.o main_test_default.o backoff.o $(ALLOCATOR)
	gcc $(CFLAGS) -o test_queue_lf_smr.exe main.c queue_lf_smr.o smr.o test_queue.o util.o random.o main_test_default.o backoff.o $(ALLOCATOR)

test_queue_lf_lfrc: test_queue.o queue_lf_lfrc.o util.o lfrc.o main.c random.o main_test_default.o backoff.o
	gcc $(CFLAGS) -o test_queue_lf_lfrc.exe main.c queue_lf_lfrc.o lfrc.o test_queue.o util.o random.o main_test_default.o backoff.o

