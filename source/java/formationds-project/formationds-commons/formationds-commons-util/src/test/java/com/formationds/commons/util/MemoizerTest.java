/*
 * Copyright (c) 2016, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.commons.util;

import java.time.Duration;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import java.util.function.Function;
import java.util.function.Supplier;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.Assert;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import com.formationds.commons.util.Memoizer.MemoizedCache;

public class MemoizerTest {

    private static final Logger logger = LogManager.getLogger( MemoizerTest.class );

    @BeforeClass
    public static void setUpBeforeClass() throws Exception {
    }

    @AfterClass
    public static void tearDownAfterClass() throws Exception {
    }

    @Before
    public void setUp() throws Exception {
    }

    @After
    public void tearDown() throws Exception {
        Memoizer.clearAll();
         i = 0;
         tpe.shutdownNow();
    }

    // hack a pool for submitting a bunch of sleep tasks concurrently
    private ThreadPoolExecutor tpe = (ThreadPoolExecutor)Executors.newFixedThreadPool( 100 );

    static final long SLEEP_TASK_SLEEP_TIME = 10;
    static final long CACHED_TASK_MAX_SLEEP_TIME = SLEEP_TASK_SLEEP_TIME / 2;

    static int i = 0;
    public static final Supplier<Integer> sleepySupplier = () -> {
        try { Thread.sleep( SLEEP_TASK_SLEEP_TIME ); }
        catch (InterruptedException ignore) {}
        return i++;
    };

    public static final Function<Integer,Integer> sleepyFunction = (j) -> {
        try { Thread.sleep( SLEEP_TASK_SLEEP_TIME ); }
        catch (InterruptedException ignore) {}
        return j;
    };

    @Test
    public void testMemoizeSupplierOfU() {
        Supplier<Integer> msup = Memoizer.memoize( sleepySupplier );

        Timed<Integer> t1 = timedOp(msup);
        Assert.assertTrue( t1.elapsedMS() >= SLEEP_TASK_SLEEP_TIME  );

        for (int i = 0; i < 100; i++) {
            Timed<Integer> t2 = timedOp(msup);
            Assert.assertTrue( t2.elapsedMS() < CACHED_TASK_MAX_SLEEP_TIME  );
            Assert.assertEquals( Integer.valueOf( 0 ), t2.result() );
        }

        // for a supplier cache, there is only ever 1 entry
        Assert.assertEquals("Expecting exactly 1 entry in the cache", 1, Memoizer.size((MemoizedCache)msup));

        // if we memoize a second supplier this would be 2, but we only have 1 here.
        Assert.assertEquals("Expecting exactly 1 entry in all caches", 1, Memoizer.size());

    }

    @Test
    public void testMemoizeSupplierOfUExpiration() throws Exception {
        Supplier<Integer> msup = Memoizer.memoize( Duration.ofMillis( 100 ), sleepySupplier );

        Timed<Integer> t1 = timedOp(msup);
        Assert.assertTrue( t1.toString(), t1.elapsedMS() >= SLEEP_TASK_SLEEP_TIME  );

        // each execution here (up to the expiration, which should not be hit here)
        // should be retrieved from the memoized cache and should return the original
        // value (0).
        // What is a reasonable/reliable expectation for cache lookup time, but not too long.
        // was using 2, but that failed sometimes (when running laptop on battery)
        for (int i = 0; i < 100; i++) {
            Timed<Integer> t2 = timedOp(msup);
            Assert.assertTrue( i + "[" + t2.elapsedMS()  + "]",
                               t2.elapsedMS() < CACHED_TASK_MAX_SLEEP_TIME  );
            Assert.assertEquals( i + ": ",  Integer.valueOf( 0 ), t2.result() );
        }

        // for a supplier cache, there is only ever 1 entry
        Assert.assertEquals("Expecting exactly 1 entry in the cache", 1, Memoizer.size((MemoizedCache)msup));

        // if we memoize a second supplier this would be 2, but we only have 1 here.
        Assert.assertEquals("Expecting exactly 1 entry in all caches", 1, Memoizer.size());

        // wait expiration plus the "sleepy supplier time" (10)
        Thread.sleep( 200 );

        Timed<Integer> t3 = timedOp(msup);
        Assert.assertTrue( t3.toString(), t3.elapsedMS() >= SLEEP_TASK_SLEEP_TIME  );
        Assert.assertTrue( t3.toString(), 1 == t3.result() );
    }

    /**
     * Validate that for the same argument, we always get the same result and that
     * the first execution of the function takes >= 100ms (hardcoded in sleepyFunction)
     * and each subsequent execution takes <= 1ms because it is coming out of the cache.
     *
     * This does not test expiration.
     *
     * @throws InterruptedException
     *
     */
    @Test
    public void testMemoizeFunction() throws InterruptedException {

        Function<Integer,Integer> mfunc = Memoizer.memoize( sleepyFunction );

        logger.debug( "Test executiong of Memoized function.  Our \"sleepy\" function" +
                " will sleep for a specified amount of time and then return the" +
                " result." );
        Timed<Integer> t1 = timedOp(1, mfunc);
        Assert.assertTrue( t1.toString(), t1.elapsedMS() >= SLEEP_TASK_SLEEP_TIME  );
        Assert.assertTrue( t1.toString(), 1 == t1.result() );

        for (int i = 0; i < 100; i++) {
            Timed<Integer> t2 = timedOp(1, mfunc);
            Assert.assertTrue( t2.toString(), t2.elapsedMS() <= 1  );
        }

        logger.debug( "Executing function 100 times with different arguments" );
        logger.debug( "Expecting cache to contain 100 items, 1 for each argument" );
        CountDownLatch c = new CountDownLatch( 100 );
        // so now test other args to the function.  ???start at something other than 1
        // which is already cached and would mess up the countdown (the function won't get exec'd)
        for (int n = 0; n < 100; n++) {
            final int x = n;
            CompletableFuture.runAsync( () -> {
                Timed<Integer> t2 = timedOp(x, mfunc);
                if ( x != 1 ) Assert.assertTrue( t2.elapsedMS() >= SLEEP_TASK_SLEEP_TIME  );
                Assert.assertTrue( t2.toString(), x == t2.result() );
                c.countDown();
            }, tpe );
        }

        // wait for all of them to complete
        while ( !c.await(SLEEP_TASK_SLEEP_TIME, TimeUnit.MILLISECONDS) ) {
            logger.debug( "Waiting for " + c.getCount() + " tasks." );
        }

        Assert.assertEquals("Expecting exactly 100 entries in the cache", 100, Memoizer.size((MemoizedCache)mfunc));
        Assert.assertEquals("Expecting exactly 100 entries in all caches", 100, Memoizer.size());

        // and subsequent execs with other args return from cache
        for (int i = 0; i < 100; i++) {
            Timed<Integer> t2 = timedOp(i, mfunc);
            Assert.assertTrue( t2.toString(), t2.elapsedMS() <= CACHED_TASK_MAX_SLEEP_TIME  );
        }
    }

    @Test
    public void testMemoizeFunctionExpiration() throws Exception {

        long expireMillis = 1000;
        long start = System.nanoTime();
        Function<Integer,Integer> mfunc = Memoizer.memoize( Duration.ofMillis( expireMillis ),
                                                            sleepyFunction );

        Timed<Integer> t1 = timedOp(1, mfunc);
        Assert.assertTrue( t1.toString(), t1.elapsedMS() >= SLEEP_TASK_SLEEP_TIME  );
        Assert.assertTrue( t1.toString(), 1 == t1.result() );

        for (int i = 0; i < 100; i++) {
            Timed<Integer> t2 = timedOp(1, mfunc);
            Assert.assertTrue( t2.toString(), t2.elapsedMS() <= CACHED_TASK_MAX_SLEEP_TIME  );
        }

        CountDownLatch c = new CountDownLatch( 100 );
        // so now test other args to the function. 1 is already cached and the
        // function won't get exec'd, so account for that.
        for (int n = 0; n < 100; n++) {
            final int x = n;
            CompletableFuture.runAsync( () -> {
                try {
                    Timed<Integer> t2 = timedOp(x, mfunc);
                    if (x != 1) Assert.assertTrue( t2.toString(), t2.elapsedMS() >= SLEEP_TASK_SLEEP_TIME  );
                    Assert.assertTrue( t2.toString(), x == t2.result() );
                } finally {
                    c.countDown();
                }
            }, tpe );
        }

        // wait for all of them to complete
        while ( !c.await(SLEEP_TASK_SLEEP_TIME, TimeUnit.MILLISECONDS) ) {
           logger.debug( "Waiting for " + c.getCount() + " tasks." );
        }

        long elapsed = System.nanoTime() - start;
        long diff = expireMillis - TimeUnit.NANOSECONDS.toMillis( elapsed );
        if ( diff > 0 ) {

            for (int i = 0; i < 100; i++) {
                Timed<Integer> t2 = timedOp(i, mfunc);
                Assert.assertTrue( t2.toString(), t2.elapsedMS() <= CACHED_TASK_MAX_SLEEP_TIME  );
            }

            elapsed = System.nanoTime() - start;
            diff = expireMillis - TimeUnit.NANOSECONDS.toMillis( elapsed );
            if ( diff > 0 ) {
                logger.debug( "waiting for expiration... " + (2 * (diff + SLEEP_TASK_SLEEP_TIME + 10)) );
                Thread.sleep( 2 * (diff + SLEEP_TASK_SLEEP_TIME + 10) );
            }
        }

        // cache has expired for all values, so any execs should recompute and take the SLEEP_TASK_SLEEP_TIME
        for (int i = 0; i < 5; i++) {
            Timed<Integer> t2 = timedOp(i, mfunc);
            Assert.assertTrue(i + ": " + t2.toString(), t2.elapsedMS() >= SLEEP_TASK_SLEEP_TIME  );
        }
    }

    @Test
    public void testPurgeExpired() throws InterruptedException, ExecutionException {
        long expireMillis = 150;
        Function<Integer,Integer> mfunc = Memoizer.memoize( Duration.ofMillis( expireMillis ),
                                                            sleepyFunction );

        for (int i = 0; i < 100; i++) {
            timedOp(i, mfunc);
        }

        Thread.sleep( expireMillis * 2 );

        logger.debug( "Purging expired entries.  Expect 0 will remain." );
        CompletableFuture<Void> cf = Memoizer.purgeExpired();
        cf.get();
        Assert.assertEquals("Expecting exactly 0 entries in the cache after purge", 0, Memoizer.size((MemoizedCache)mfunc));
        Assert.assertEquals("Expecting exactly 0 entries in all caches after purge", 0, Memoizer.size());
    }

    private class Timed<R> {
        private final long elapsed;
        private final R result;
        Timed(long elapsed, R result) {
            this.elapsed = elapsed;
            this.result = result;
        }
        // elapsed time in nanoseconds
        public long elapsed() { return elapsed; }
        public long elapsedMS() { return TimeUnit.NANOSECONDS.toMillis(  elapsed() ); }
        public R result() { return result; }
        public String toString() { return String.format( "%s [%d ns (%d ms)]", result, elapsed(), elapsedMS() ); }
    }

    public <R> Timed<R> timedOp( Supplier<R> s ) {
        long start = System.nanoTime();
        R result = s.get();
        return new Timed<R>(System.nanoTime() - start, result);
    }

    public <T,R> Timed<R> timedOp( T arg, Function<T,R> f ) {
        long start = System.nanoTime();
        R result = f.apply(arg);
        return new Timed<R>(System.nanoTime() - start, result);
    }

}
