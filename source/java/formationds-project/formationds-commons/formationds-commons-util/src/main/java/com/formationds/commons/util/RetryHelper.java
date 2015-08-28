/**
 * Copyright (c) 2014 Formation Data Systems.  All rights reserved.
 */

package com.formationds.commons.util;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Random;
import java.util.UUID;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

public class RetryHelper {

    public static final Logger logger = LoggerFactory.getLogger( RetryHelper.class );

    static final Random rng = new Random( System.currentTimeMillis() );

    /**
     *
     * @param timeout the connection timeout
     * @param unit the timeout units.
     * @param retryable the retryable action
     *
     * @return the object representing the underlying connection
     *
     * @throws InterruptedException if retry attempt is interrupted
     * @throws TimeoutException if the specified timeout expires before a connection is obtained.
     * @throws IllegalStateException if an error occurs connecting
     */
    public static <T> T retry( long timeout, TimeUnit unit, SupplierWithExceptions<T> retryable )
        throws InterruptedException, TimeoutException {

        return retry( UUID.randomUUID().toString(), timeout, unit, retryable );

    }

        /**
         * @param id an identifier for the retryable operation, used for logging and tracking
         * @param timeout the connection timeout
         * @param unit the timeout units.
         * @param retryable the retryable action
         *
         * @return the object representing the underlying connection
         *
         * @throws InterruptedException if retry attempt is interrupted
         * @throws TimeoutException if the specified timeout expires before a connection is obtained.
         * @throws IllegalStateException if an error occurs connecting
         */
    public static <T> T retry( String id, long timeout, TimeUnit unit, SupplierWithExceptions<T> retryable )
        throws InterruptedException, TimeoutException {

        CompletableFuture<T> f = asyncRetry( id, retryable );
        try {

            // if the timeout is zero, use getNow().  Otherwise wait for up to the specified amount of time.
            // (this is required because CompletableFuture will throw a TimeoutException if the timeout reaches 0, even
            // if it is completed.  An alternative is to convert the timeout to something minuscule like 1 nano)
            if ( timeout <= 0 ) {
                T db = f.getNow( null );
                if ( db == null ) {
                    throw new IllegalStateException( "Retryable action failed." );
                }
                return db;
            }
            return f.get( timeout, unit );

        } catch (ExecutionException ee ) {

            Throwable throwable = ee.getCause();
            if ( throwable instanceof RuntimeException ) {
                throw (RuntimeException) throwable;
            } else {
                throw new IllegalStateException( "Retryable action failed.", throwable );
            }

        } finally {
            f.cancel( true );
        }
    }

    /**
     *
     * @param retryable the connection factory
     * @param <T> the type of factory/supplier
     *
     * @return a completable future that will retry the connection indefinitely until successful
     */
    public static <T> CompletableFuture<T> asyncRetry( SupplierWithExceptions<T> retryable ) {
        return asyncRetry( UUID.randomUUID().toString(), retryable );
    }

    /**
     *
     * @param id an identifier for the retryable operation, used for logging and tracking
     * @param retryable the connection factory
     * @param <T> the type of factory/supplier
     *
     * @return a completable future that will retry the connection indefinitely until successful
     */
    public static <T> CompletableFuture<T> asyncRetry( String id, SupplierWithExceptions<T> retryable ) {
        final int minDelay = 500;
        final int maxDelay = 10000;
        return asyncRetry( id, minDelay, maxDelay, retryable );
    }

    /**
     *
     * @param minDelay the minimum delay between retries
     * @param maxDelay the max delay between retries
     * @param retryable the factory/supplier
     * @param <T> the type of object returned by the retryable supplier
     *
     * @return a completable future that will retry the connection indefinitely until successful
     */
    public static <T> CompletableFuture<T> asyncRetry( int minDelay,
                                                       int maxDelay,
                                                       SupplierWithExceptions<T> retryable ) {

        return asyncRetry( UUID.randomUUID().toString(), minDelay, maxDelay, retryable );

    }

    /**
     * @param id an identifier for the retryable operation, used for logging and tracking
     * @param minDelay the minimum delay between retries
     * @param maxDelay the max delay between retries
     * @param retryable the connection factory
     * @param <T> the type of connection
     *
     * @return a completable future that will retry the connection indefinitely until successful
     */
    public static <T> CompletableFuture<T> asyncRetry( String id,
                                                       int minDelay,
                                                       int maxDelay,
                                                       SupplierWithExceptions<T> retryable ) {

        // TODO: this is not ideal in that each retryable operation ties up a thread for an unbounded
        // amount of time.  It would be better to use a single scheduled executor service to manage
        // the delay between retries and execution in the scheduled executor pool.
        return CompletableFuture.supplyAsync( () -> {
            int cnt = 0;
            T conn = null;
            do {
                try {

                    conn = retryable.supply();

                } catch ( Exception e ) {

                    conn = null;
                    try {

                        final int delay = Math.max( minDelay, rng.nextInt( maxDelay ) );
                        if ( cnt % 20 == 0 ) {
                            logger.trace( "[{}] {} retry attempts failed.  Server is not ready.  Waiting {} ms before retrying",
                                          id,
                                          (cnt == 0 ? "" : cnt),
                                          delay );
                        }
                        Thread.sleep( delay );
                    } catch ( InterruptedException ie ) {

                        // reset interrupt
                        Thread.currentThread().interrupt();
                        throw new IllegalStateException( "Retry interrupted." );
                    }
                }
                cnt++;
            }
            while ( conn == null );

            return conn;
        } );
    }
}
