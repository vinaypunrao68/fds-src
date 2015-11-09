package com.formationds.util.executor;

import java.util.ArrayDeque;
import java.util.Queue;
import java.util.concurrent.Executor;

public class SubExecutor implements Executor {
    private Executor parent;
    private int threadMax;
    private int current;
    private Queue<Runnable> queue;

    public SubExecutor(Executor parent, int threadMax) {
        this.parent = parent;
        this.threadMax = threadMax;
        this.queue = new ArrayDeque<>();
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
        try {
            runnable.run();
        } finally {
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
