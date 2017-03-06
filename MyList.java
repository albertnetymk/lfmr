import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;

class MyList {
	public final static class HarrisList {
		// retrieved from
		// https://github.com/wala/MemSAT/blob/master/com.ibm.wala.memsat.testdata/source/data/concurrent/HarrisList.java


		private final Node head, tail;

		/**
		 * Constructs an empty Harris list.
		 */
		public HarrisList() {
			head = new Node(Short.MIN_VALUE);
			tail = new Node(Short.MAX_VALUE);
			head.next.set(tail);
		}

		/**
		 * Adds the given key to this list if not already present.
		 * Returns true if the list was modified as a result of
		 * this operation; otherwise returns false.
		 * @return true if the list was modified as a result of
		 * this operation; otherwise returns false.
		 */
		public boolean insert(long key) {
			Node newNode = new Node(key);
			Node rightNode, leftNode;

			do {
				Pair pair = search(key);
				leftNode = pair.fst;
				rightNode = pair.snd;

				if (rightNode != tail && rightNode.key==key) {
					return false;
				}
				newNode.next = new AtomicReference/*<Node>*/(rightNode);
				if (leftNode.next.compareAndSet(rightNode, newNode))
					return true;
			} while(true);
		}

		/**
		 * Removes the given key from this list, if present.
		 * Returns true if the list was modified as a result of
		 * this operation; otherwise returns false.
		 * @return true if the list was modified as a result of
		 * this operation; otherwise returns false.
		 */
		public boolean delete(long key) {
			Node rightNode, rightNodeNext, leftNode;
			do {
				Pair pair = search(key);
				leftNode = pair.fst;
				rightNode = pair.snd;

				if (rightNode==tail || rightNode.key!=key) {
					return false;
				}

				rightNodeNext = rightNode.next();
				if (!rightNode.isMarked()) {
					if (rightNode.isMarked.compareAndSet(false, true)) {
						break;
					}
				}

			} while(true);

			if (!leftNode.next.compareAndSet(rightNode, rightNodeNext)) {
				search(rightNode.key);
			}

			return true;
		}

		public boolean find(long key) {
			Pair pair = search(key);
			Node right_node = pair.snd;
			if (right_node == tail || right_node.key != key) {
				return false;
			}
			return true;
		}

		/**
		 * Returns a pair of adjacent unmarked nodes
		 * such that the key of the left node is less
		 * than searchKey and the key of the right node
		 * is greater than or equal to searchKey.
		 * @return a pair of adjacent unmarked nodes
		 * such that the key of the left node is less
		 * than searchKey and the key of the right node
		 * is greater than or equal to searchKey.
		 */
		private Pair/*<Node, Node>*/ search(long searchKey) {
			Node leftNode = null, leftNodeNext = null, rightNode = null;

search_again:
			do {
				Node t = head;
				Node tNext = head.next();

				do {
					if (!t.isMarked.get()) {
						leftNode = t;
						leftNodeNext = t.next();
					}
					t = tNext;
					if (t==tail) break;
					tNext = t.next();
				} while (t.isMarked() || t.key < searchKey);
				rightNode = t;

				if (leftNodeNext == rightNode) {
					if (rightNode != tail && rightNode.isMarked())
						continue search_again;
					else
						return new Pair(leftNode,rightNode);
				}

				if (leftNode.next.compareAndSet(leftNodeNext, rightNode)) {
					if (rightNode != tail && rightNode.isMarked())
						continue search_again;
					else
						return new Pair(leftNode,rightNode);
				}
			} while (true);
		}


		/**
		 * Node in the linked list.
		 */
		static final class Node {
			long key;
			AtomicReference/*<Node>*/ next;
			AtomicBoolean isMarked;

			Node(long key) {
				this.key = key;
				this.next = new AtomicReference/*<Node>*/();
				this.isMarked = new AtomicBoolean(false);
			}

			boolean isMarked() { return isMarked.get(); }

			Node next() { return (Node)next.get(); }
		}

		/**
		 * A pair of nodes.
		 * @author Emina Torlak
		 */
		static final class Pair {
			final Node fst;
			final Node snd;

			Pair(Node fst, Node snd) {
				this.fst = fst;
				this.snd = snd;
			}
		}
	}
	static volatile int state = 0;
	static class Random {
		long seed;

		public Random(long seed) {
			this.seed = seed;
		}

		public long next() {
			long x, hi, lo, t;
			/*
			 * Compute x[n + 1] = (7^5 * x[n]) mod (2^31 - 1).
			 * From "Random number generators: good ones are hard to find",
			 * Park and Miller, Communications of the ACM, vol. 31, no. 10,
			 * October 1988, p. 1195.
			 */
			x = seed;
			hi = x / 127773;
			lo = x % 127773;
			t = 16807 * lo - 2836 * hi;
			if (t <= 0) {
				t += 0x7fffffff;
			}
			seed = t;
			return t;
		}
	}

	static class Agent implements Runnable {
		static int id;
		static int n_search = 100 - n_update;
		static HarrisList list;

		public Agent(int id, HarrisList list) {
			this.id = id;
			this.list = list;
		}

		@Override
		public void run() {
			Random rand = new Random(System.nanoTime());

			int N = 100;
			long r, key, action, ins_del;
			long amask = (1<<10)-1;
			while (state == 0) ;

			while (state != 2) {
				for (int i = 0; i < N; ++i) {
					r = rand.next();
					ins_del = r & 1;
					r >>= 1;
					action = ((r & amask) % 101);
					r >>= 10;
					key = r % n_keys;

					if (action <= n_search) {
						list.search(key);
					} else if (ins_del == 0) {
						list.delete(key);
					} else {
						list.insert(key);
					}
				}
				MyList.ops[id] += N;
			}
		}
	}

	static Thread[] threads;
	static int[] ops;
	static int n_ms;
	static int n_update;
	static int n_elements;
	static int n_threads;
	static int n_keys;

	public static void main(String[] args) throws Exception {
		if (args.length != 4) {
			System.out.println("Usage: nmilli nupdate nelements nthreads");
			System.exit(-1);
		}

		n_ms = Integer.parseInt(args[0]);
		n_update = Integer.parseInt(args[1]);
		n_elements = Integer.parseInt(args[2]);
		n_threads = Integer.parseInt(args[3]);
		n_keys = n_elements * 2;

		threads = new Thread[n_threads];

		// warm up jvm
		for (int j = 0; j < 5; ++j)
		{
			ops = new int[n_threads];
			HarrisList list = new HarrisList();

			for (int i = 0; i < n_elements; ++i) {
				list.insert((long)i*2);
			}

			state = 0;
			for (int i = 0; i < n_threads; ++i) {
				threads[i] = new Thread(new Agent(i, list));
				threads[i].start();
			}

			state = 1;
			long start_time = System.nanoTime();

			Thread.sleep(1000);

			state = 2;

			for (int i = 0; i < n_threads; ++i) {
				threads[i].join();
			}
			long end_time = System.nanoTime();
			long total_ops = 0;

			for (int i = 0; i < n_threads; ++i) {
				total_ops += ops[i];
			}

			int co = n_threads <= 64 ? n_threads : 1;
		}
		System.gc();
		for (int j = 0; j < 1; ++j)
		{
			ops = new int[n_threads];
			HarrisList list = new HarrisList();

			for (int i = 0; i < n_elements; ++i) {
				list.insert((long)i*2);
			}

			state = 0;
			for (int i = 0; i < n_threads; ++i) {
				threads[i] = new Thread(new Agent(i, list));
				threads[i].start();
			}

			long start_time = System.nanoTime();
			state = 1;

			Thread.sleep(n_ms);

			state = 2;

			for (int i = 0; i < n_threads; ++i) {
				threads[i].join();
			}
			long end_time = System.nanoTime();
			long total_ops = 0;

			for (int i = 0; i < n_threads; ++i) {
				if (Long.MAX_VALUE - total_ops > ops[i]) {
					total_ops += ops[i];
				} else {
					System.out.print("error");
				}

			}

			int co = n_threads <= 64 ? n_threads : 1;
			System.out.println(co * (end_time-start_time) / total_ops);
		}
	}
}

