import java.util.*;
import java.util.concurrent.atomic.*;
// import java.lang.management.*;

class MyQueue {
    static class MSQueue<E> {
        private final MSQueue.Node<E> dummy
            = new MSQueue.Node<>(null); //the initial "dummy" node
        private final AtomicReference<MSQueue.Node<E>> head
            = new AtomicReference<>(dummy); //reference to queue head
        private final AtomicReference<MSQueue.Node<E>> tail
            = new AtomicReference<>(dummy); //reference to queue tail

        public boolean offer(E item) {
            MSQueue.Node<E> newNode = new MSQueue.Node<>(item); //new node created

            while (true) {
                MSQueue.Node<E> curTail = tail.get(); //"tail" value is read
                MSQueue.Node<E> tailNext = curTail.next.get();//next of "tail"

                if (curTail == tail.get()) {
                    if (tailNext != null) { // queue in intermediate state...
                        tail.compareAndSet(curTail, tailNext); //advance tail
                    } else { // queue in quiescent state...
                        // try inserting the new node
                        if (curTail.next.compareAndSet(null, newNode)) {
                            // insertion succeeded, try advancing tail
                            tail.compareAndSet(curTail, newNode);
                            return true;
                        }
                    }
                }
            }
        }

        public E poll() {
            while (true) {
                MSQueue.Node<E> h = head.get();
                MSQueue.Node<E> t = tail.get();
                MSQueue.Node<E> h_next = h.next.get();
                if (h_next == null) {
                    return null;
                }
                // if (h == t) {
                //     tail.compareAndSet(t, h_next);
                //     continue;
                // }
                if (head.compareAndSet(h, h_next)) {
                    return h_next.item;
                }
            }
        }

        private static class Node<E> { //inner class to support a "node"
            final E item;
            AtomicReference<MSQueue.Node<E>> next = new AtomicReference<>(null); //reference to queue head

            public Node(E item) {
                this.item = item;
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
        static MSQueue<Long> q;

        public Agent(int id, MSQueue<Long> q) {
            this.id = id;
            this.q = q;
        }

        @Override
        public void run() {
            Random rand = new Random(System.nanoTime());

            int N = 100;
            long r, key, action;
            while (state == 0) ;

            while (state != 2) {
                for (int i = 0; i < N; ++i) {
                    r = rand.next();
                    //r = i;
                    action = r & 1;
                    r >>= 1;
                    key = r % n_keys;

                    if (action == 1) {
                        // System.out.println("in");
                        q.offer(key);
                    } else {
                        // System.out.println("out");
                        q.poll();
                    }
                }
                MyQueue.ops[id] += N;
            }
        }
    }

    static Thread[] threads;
    static int[] ops;
    static int n_ms;
    static int n_elements;
    static int n_threads;
    static int n_keys;

    public static void main(String[] args) throws Exception {
        if (args.length != 3) {
            System.out.println("Usage: nmilli nelements nthreads");
            System.exit(-1);
        }

        n_ms = Integer.parseInt(args[0]);
        n_elements = Integer.parseInt(args[1]);
        n_threads = Integer.parseInt(args[2]);
        n_keys = n_elements * 2;

        threads = new Thread[n_threads];

        // warm up jvm
        for (int j = 0; j < 5; ++j)
        {
        ops = new int[n_threads];
        MSQueue<Long> q = new MSQueue<>();

        for (int i = 0; i < n_elements; ++i) {
            q.offer((long)i*2);
        }

        state = 0;
        for (int i = 0; i < n_threads; ++i) {
            threads[i] = new Thread(new Agent(i, q));
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
        MSQueue<Long> q = new MSQueue<>();

        for (int i = 0; i < n_elements; ++i) {
            q.offer((long)i*2);
        }

        state = 0;
        for (int i = 0; i < n_threads; ++i) {
            threads[i] = new Thread(new Agent(i, q));
            threads[i].start();
        }

        long start_time = System.nanoTime();
        state = 1;

        // System.out.println("want to sleep " + n_ms/1000);
        Thread.sleep(n_ms);
        // System.out.println("sleep time is " + (System.nanoTime()-start_time)/1000000);

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
