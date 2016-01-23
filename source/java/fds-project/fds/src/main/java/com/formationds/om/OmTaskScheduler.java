/**
 *
 */
package com.formationds.om;

import java.util.concurrent.Callable;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import com.formationds.commons.util.thread.ThreadFactories;
import com.formationds.om.helper.SingletonConfiguration;

/**
 * Provides an OM Service Task scheduler built on top of the {@link ScheduledExecutorService}.
 * <p/>
 * To avoid saturation of the scheduled pool, it is important that the execution time of
 * scheduled tasks are relatively short-running for each execution of the task.  Long running
 * tasks are likely to use up all of the threads allocated to the pool and starve other tasks.
 *
 */
// TODO: consider adding a second executor service specifically for potentially long running tasks,
// can wrap incoming tasks expected to be long-running in a runnable that simply delegates to the
// long running executor pool when the scheduled task runs (taking care to either handle or document
// fixed-delay semantics to prevent overlapping execution).
public enum OmTaskScheduler {

    INSTANCE;

    /**
     * @see ScheduledExecutorService#schedule(Runnable, long, TimeUnit)
     */
    public static ScheduledFuture<?> schedule( Runnable command, long delay, TimeUnit unit ) {
        return INSTANCE.scheduler.schedule( command, delay, unit );
    }

    /**
     * @see ScheduledExecutorService#schedule(Callable, long, TimeUnit)
     */
    public static <V> ScheduledFuture<V> schedule( Callable<V> callable, long delay, TimeUnit unit ) {
        return INSTANCE.scheduler.schedule( callable, delay, unit );
    }

    /**
     * @see ScheduledExecutorService#scheduleAtFixedRate(Runnable, long, long, TimeUnit)
     */
    public static ScheduledFuture<?> scheduleAtFixedRate( Runnable command,
                                                          long initialDelay,
                                                          long period,
                                                          TimeUnit unit ) {
        return INSTANCE.scheduler.scheduleAtFixedRate( command, initialDelay, period, unit );
    }

    /**
     * @see ScheduledExecutorService#scheduleWithFixedDelay(Runnable, long, long, TimeUnit)
     */
    public static ScheduledFuture<?> scheduleWithFixedDelay( Runnable command,
                                                             long initialDelay,
                                                             long delay,
                                                             TimeUnit unit ) {
        return INSTANCE.scheduler.scheduleWithFixedDelay( command, initialDelay, delay, unit );
    }

    /**
     * @see ScheduledExecutorService#execute(Runnable)
     */
    public static void execute(Runnable command) { INSTANCE.scheduler.execute(command); }

    /**
     * @see ScheduledExecutorService#submit(Callable)
     */
    public static <T> Future<T> submit(Callable<T> task) { return INSTANCE.scheduler.submit(task); }

    /**
     * @see ScheduledExecutorService#submit(Runnable, Object)
     */
    public static <T> Future<T> submit(Runnable task, T result) { return INSTANCE.scheduler.submit(task, result); }

    /**
     * @see ScheduledExecutorService#submit(Runnable)
     */
    public static Future<?> submit(Runnable task) { return INSTANCE.scheduler.submit(task);	}

    public static final String MIN_POOL_SIZE_KEY = "fds.om.task_scheduler.min";
    private final ScheduledExecutorService scheduler;

    private final int min;

    private OmTaskScheduler() {

        min = SingletonConfiguration.getIntValue( MIN_POOL_SIZE_KEY, 10 );
        scheduler = Executors.newScheduledThreadPool(min, ThreadFactories.newThreadFactory("om-task-sched", true));
    }

    void shutdown() { scheduler.shutdown(); }
    void shutdownNow() { scheduler.shutdownNow(); }
    boolean awaitTermination(long timeout, TimeUnit unit) throws InterruptedException {
        return scheduler.awaitTermination(timeout, unit);
    }

    int getThreadPoolSize() { return ((ScheduledThreadPoolExecutor)scheduler).getPoolSize(); }
    int getMin() { return min; }
}
