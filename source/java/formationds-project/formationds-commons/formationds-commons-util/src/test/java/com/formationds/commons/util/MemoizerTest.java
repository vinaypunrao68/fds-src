/*
 * Copyright (c) 2016, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.commons.util;

import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.function.Function;
import java.util.function.Supplier;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.Assert;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Ignore;
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
         supplierInc = 0;
    }

    int supplierInc = 0;

    // Supplier that increments a value.  When used with the memoizer,
    // it should always return the initial value and only increment
    // upon expiration (or purge).
    public final Supplier<Integer> incrementingSupplier = () -> {
        return supplierInc++;
    };

    /**
     * @author dave
     *
     */
    private static class Result {
        private final Integer i;
        private final Instant ts;
        public Result(Integer i, Instant ts) {
            this.i = i;
            this.ts = ts;
        }
        public Instant ts() { return ts; }
        public Integer input() { return i; }
        /* (non-Javadoc)
         * @see java.lang.Object#hashCode()
         */
        @Override
        public int hashCode() {
            return Objects.hash( i, ts );
        }

        /* (non-Javadoc)
         * @see java.lang.Object#equals(java.lang.Object)
         */
        @Override
        public boolean equals( Object obj ) {
            if ( this == obj ) return true;
            if ( obj == null ) return false;
            Result other = (Result) obj;
            return Objects.equals( i, other.i ) &&
                   Objects.equals( ts, other.ts );
        }

        /* (non-Javadoc)
         * @see java.lang.Object#toString()
         */
        @Override
        public String toString() {
            return String.format( "Result [i=%s, ts=%s]", i, ts );
        }
    }


    // Function returns a Pair containing the input argument and a timestamp (Instant)
    // When memoized and returned from cache, the timestamp will not change for the input.
    // After an expiration, the timestamp will change.  We will use these side-effects to
    // test the memoization.
    public final Function<Integer, Result> timestampFunction = (j) -> {
        return new Result( j, Instant.now() );
    };

    @Test
    @Ignore
    public void testMemoizeSupplierOfU() {
        Supplier<Integer> msup = Memoizer.memoize( incrementingSupplier );

        Integer t1 = msup.get();
        Assert.assertEquals( "Value should be 0", Integer.valueOf( 0 ), t1  );

        // since there is no expiration we can execute the supplier
        // any number of times and get the same result.
        for (int i = 0; i < 100; i++) {
            Integer t2 = msup.get();
            Assert.assertEquals( "Cached value should be 0", Integer.valueOf( 0 ), t2 );
        }

        // for a supplier cache, there is only ever 1 entry
        Assert.assertEquals("Expecting exactly 1 entry in the cache", 1, Memoizer.size((MemoizedCache)msup));

        // if we memoize a second supplier this would be 2, but we only have 1 here.
        Assert.assertEquals("Expecting exactly 1 entry in all caches", 1, Memoizer.size());
    }

    @Test
    public void testMemoizeSupplierOfUExpiration() throws Exception {
        Supplier<Integer> msup = Memoizer.memoize( Duration.ofMillis( 100 ), incrementingSupplier );

        Integer t1 = msup.get();
        Assert.assertEquals( "Value should be 0", Integer.valueOf( 0 ), t1  );

        // for a supplier cache, there is only ever 1 entry
        Assert.assertEquals("Expecting exactly 1 entry in the cache", 1, Memoizer.size((MemoizedCache)msup));

        // if we memoize a second supplier this would be 2, but we only have 1 here.
        Assert.assertEquals("Expecting exactly 1 entry in all caches", 1, Memoizer.size());

        // wait twice the expiration
        Thread.sleep( 200 );

        // After expiration, we expect the value incremented
        Integer t3 = msup.get();
        Assert.assertEquals( "Expecting the supplied value to have incremented after expiration",
                             Integer.valueOf( 1 ), t3 );
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

        Function<Integer,Result> mfunc = Memoizer.memoize( timestampFunction );

        Result t1 = mfunc.apply( 0 );
        Assert.assertEquals( "Expecting a result of 0", Integer.valueOf( 0 ), t1.input() );

        Instant ts = t1.ts();

        // execute the function with the same argument a several times and validate we
        // get the same timestamp
        for (int i = 0; i < 10; i++) {
            Thread.sleep( 1 );

            Result t2 = mfunc.apply( 0 );
            Assert.assertEquals( "Expecting a result of 0", Integer.valueOf( 0 ), t2.input() );
            Assert.assertEquals( "Expecting the timestamp returned is identical to original execution", ts, t2.ts() );
        }

        logger.debug( "Executing function 10 times with different arguments" );
        logger.debug( "Expecting cache to contain 10 items, 1 for each argument" );

        List<Result> results = new ArrayList<>();

        // so now test other args to the function.
        for (int n = 0; n < 10; n++) {
            // do a short sleep just to space out the results slightly
            Thread.sleep( 1 );

            Result t2 = mfunc.apply( n );
            Assert.assertEquals( "Expecting a result of " + n, Integer.valueOf( n ), t2.input() );

            results.add( t2 );
        }

        Assert.assertEquals("Expecting exactly 10 entries in the cache", 10, Memoizer.size((MemoizedCache)mfunc));
        Assert.assertEquals("Expecting exactly 10 entries in all caches", 10, Memoizer.size());

        // and subsequent execs with other args return from cache
        for (int i = 0; i < 10; i++) {

            Result t2 = mfunc.apply( i );

            Assert.assertEquals( "Expecting a result of " + i, Integer.valueOf( i ), t2.input() );
            Assert.assertEquals( "Expecting the timestamp returned is identical to original execution",
                                 results.get( i ).ts(),
                                 t2.ts() );
        }
    }

    @Test
    public void testMemoizeFunctionExpiration() throws Exception {

        long expireMillis = 1000;
        Function<Integer,Result> mfunc = Memoizer.memoize( Duration.ofMillis( expireMillis ), timestampFunction );

        List<Result> results = new ArrayList<>();
        for (int n = 0; n < 10; n++) {
            // do a short sleep just to space out the results slightly
            Thread.sleep( 1 );

            Result t1 = mfunc.apply( n );
            Assert.assertEquals( "Expecting a result of " + n,
                                 Integer.valueOf( n ),
                                 t1.input() );

            results.add( t1 );
        }

        Thread.sleep( expireMillis + 100 );

        // cache has expired for all values, so any execs should recompute
        for (int i = 0; i < 10; i++) {

            // do a short sleep just to space out the results slightly
            Thread.sleep( 1 );

            Result t2 = mfunc.apply( i );

            Assert.assertEquals( "Expecting a result of " + i, Integer.valueOf( i ), t2.input() );
            Assert.assertNotEquals( "Expecting the timestamp returned (" + results.get( i ).ts() + ") is not the same as the " +
                                    "original execution",
                                    results.get( i ).ts(),
                                    t2.ts() );
        }
    }

    @Test
    @Ignore
    public void testPurgeExpired() throws InterruptedException, ExecutionException {
        long expireMillis = 150;
        Function<Integer,Result> mfunc = Memoizer.memoize( Duration.ofMillis( expireMillis ),
                                                                          timestampFunction );

        for (int i = 0; i < 100; i++) {
            mfunc.apply(i);
        }
        Assert.assertEquals("Expecting exactly 100 entries in the cache before purge", 100, Memoizer.size((MemoizedCache)mfunc));

        Thread.sleep( expireMillis * 2 );

        logger.debug( "Purging expired entries.  Expect 0 will remain." );
        CompletableFuture<Void> cf = Memoizer.purgeExpired();
        cf.get();
        Assert.assertEquals("Expecting exactly 0 entries in the cache after purge", 0, Memoizer.size((MemoizedCache)mfunc));
        Assert.assertEquals("Expecting exactly 0 entries in all caches after purge", 0, Memoizer.size());
    }
}
