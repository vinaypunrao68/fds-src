package com.formationds.util.executor;

import java.util.ArrayDeque;
import java.util.Queue;
import java.util.concurrent.Executor;
import java.util.concurrent.atomic.AtomicLong;

public class SubExecutor implements Executor {
    private final String threadName;
    private Executor parent;
    private int threadMax;
    private int current;
    private Queue<Runnable> queue;
    private AtomicLong threadId;

    public SubExecutor(Executor parent, int threadMax, String threadName) {
        this.parent = parent;
        this.threadMax = threadMax;
        this.queue = new ArrayDeque<>();
        this.threadName = threadName;
        threadId = new AtomicLong(0L);
    }

    // TODO: make this lockless if performance if performance ends up being bad
    @Override
    public void execute(Runnable command) {
        synchronized (queue) {
            if(current < threadMax) {
                current++;
                parent.execute(() -> runTask(command));
            } else {
                queue.add(command);
            }
        }
    }

    private void runTask(Runnable runnable) {
        String priorName = Thread.currentThread().getName();
        Thread.currentThread().setName(threadName + "-" + threadId.getAndIncrement());
        try {
            runnable.run();
        } finally {
            Thread.currentThread().setName(priorName);
            synchronized (queue) {
                if(queue.size() > 0) {
                    Runnable r = queue.remove();
                    parent.execute(() -> runTask(r));
                } else {
                    current--;
                }
            }
        }
    }
}
