package com.formationds.nfs;

import java.util.concurrent.*;

public class BlockingRejectedExecutionHandler implements RejectedExecutionHandler {

    public static final long DEFAULT_TIMEOUT = 60;
    public static final TimeUnit DEFAULT_TIMEOUT_UNIT = TimeUnit.SECONDS;

    private long timeout = DEFAULT_TIMEOUT;
    private TimeUnit unit = DEFAULT_TIMEOUT_UNIT;

    public BlockingRejectedExecutionHandler(long timeout, TimeUnit unit) {
        this.timeout = timeout;
        this.unit = unit;
    }

    @Override
    public void rejectedExecution(Runnable r, ThreadPoolExecutor executor) {
        BlockingQueue<Runnable> queue = executor.getQueue();
        try {
            long startNanos = System.nanoTime();
            if (!executor.isShutdown()) {
                if (queue.offer(r, timeout, unit)) {
                    return;
                }

                // attempt to purge any cancelled tasks from queue and retry
                if (!executor.isShutdown()) {

                    executor.purge();
                    if (queue.offer(r)) {
                        return;
                    }
                }

                throw new RejectedExecutionException("NFS Task Queue Rejected due to saturated queue  ");
            }
        } catch (InterruptedException te) {

            throw new RejectedExecutionException("NFS Task Queue rejected task due to interruption.");

        }
    }
}