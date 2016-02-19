/*
 * Copyright (c) 2016, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.commons.util;

import java.time.Duration;
import java.time.Instant;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.Callable;
import java.util.concurrent.CancellationException;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.concurrent.FutureTask;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReadWriteLock;
import java.util.concurrent.locks.ReentrantReadWriteLock;
import java.util.function.Function;
import java.util.function.Supplier;

/**
 * Provides various implementations of Memoization for {@link Function}s and
 * {@link Supplier}s
 * <p>
 * For all function memoization implementations that utilize a
 * ConcurrentHashMap, recursive functions are likely to encounter a deadlock in
 * computeIfAbsent. Hence, do not use memoization for functions that are
 * recursive. Either re-write them iteratively or do not use memoization.
 */
public class Memoizer {

    private static final Set<MemoizedCache> caches = new HashSet<>();

    /**
     * Memoize the specified function.
     *
     * @param function
     *            the function, which must not be recursive
     *
     * @return the memoized function
     */
    public static <T, U> Function<T, U> memoize( final Function<T, U> function ) {
        return memoize( Optional.empty(), function );
    }

    /**
     *
     * @param exp
     * @param function
     *
     * @return
     */
    public static <T, U> Function<T, U> memoize( Duration exp, final Function<T, U> function ) {
        return memoize( exp == null ? Optional.empty() : Optional.of( exp ), function );
    }

    /**
     * Memoize the specified function.
     *
     * @param exp
     *            the optional expiration
     * @param function
     *            the function, which must not be recursive
     *
     * @return the memoized function
     */
    public static <T, U> Function<T, U> memoize( Optional<Duration> exp, final Function<T, U> function ) {
        Objects.requireNonNull( exp );
        MemoizedFunction<T,U> c = new MemoizedFunction<>( exp, function );
        caches.add( c );
        return c;
    }

    /**
     * Memoize the specified supplier.
     *
     * @param s
     *            the supplier
     *
     * @return the memoized supplier
     */
    public static <U> Supplier<U> memoize( final Supplier<U> s ) {
        return memoize( Optional.empty(), s );
    }

    /**
     *
     * @param expiration
     *            the expiration time after which the value should be
     *            recomputed.
     * @param s
     *            the supplier
     *
     * @return the memoized supplier
     */
    public static <U> Supplier<U> memoize( Duration expiration, final Supplier<U> s ) {
        return memoize( expiration == null ? Optional.empty() : Optional.of( expiration ), s );
    }

    /**
     *
     * @param expiration
     *            the expiration time after which the value should be
     *            recomputed.
     * @param s
     *            the supplier
     *
     * @return the memoized supplier
     */
    public static <U> Supplier<U> memoize( Optional<Duration> expiration, final Supplier<U> s ) {
        Objects.requireNonNull( expiration );
        Objects.requireNonNull( s );
        MemoizedSupplier<U> c = new MemoizedSupplier<U>( expiration, s );
        caches.add(c);
        return c;
    }

    /**
     * Purge all expired entries from memoized caches.
     */
    public static CompletableFuture<Void> purgeExpired() {
        return CompletableFuture.runAsync( () -> {
            for (MemoizedCache c : caches) {
                c.purgeExpired();
            }
        } );
    }

    /**
     * Clear all of the memoized caches.
     */
    static CompletableFuture<Void> clearAll() {
        return CompletableFuture.runAsync( () -> {
            for (MemoizedCache c : caches) {
                c.clear();
            }
        } );
    }

    /**
     * @return the total size of all of the memoized caches.
     */
    public static long size() {
        long total = 0;
        for (MemoizedCache c : caches) {
            total += c.size();
        }
        return total;
    }

    /**
     *
     * @param memoizedFunc
     * @return the size of the specified memoized function (as a MemoizedCache)
     */
    public static long size(MemoizedCache memoizedFunc) {
        return memoizedFunc.size();
    }

    /**
     * Interface implemented by memoized caches to support clear, purge, and size
     */
    public static interface MemoizedCache {
        public void clear();
        public void purgeExpired();
        public int size();
    }

    /**
     * Store a computed value along with a last updated timestamp for detecting
     * expiration.
     *
     * @param <U>
     */
    private static class Computed<U> {
        private final U value;
        private final Instant lastUpdated = Instant.now();

        Computed( U v ) {
            this.value = v;
        }

        U get() {
            return value;
        }

        Instant lastUpdated() {
            return this.lastUpdated;
        }

        Duration sinceUpdate() {
            return Duration.between( lastUpdated(), Instant.now() );
        }

        /**
         *
         * @param period
         * @return true if the optional period is present and the period is less than
         * the elapsed time since the last update
         */
        boolean isExpired( Optional<Duration> period ) {
            return period.isPresent() && period.get().compareTo( sinceUpdate() ) < 0;
        }
    }

    /**
     * Memoized Supplier.
     *
     * This implementation relies on a ConcurrentHashMap for ensuring thread
     * safety and preventing multiple executions of the Supplier due to a
     * contended cache miss. This ConcurrentMap's computeIfAbsent functionality
     * that guarantees the value for the same key will not be computed more than
     * once.
     *
     * The outcome is identical to the {@link MemoizedSupplierFutureCache}
     * implementation, which instead uses a technique described in Brian Goetz's
     * Java Concurrency in Practice based on a
     * {@link java.util.concurrent.Future}.
     *
     * In both of these implementations, the use of a ConcurrentMap adds
     * overhead for the map that is actually unnecessary for the purposes of
     * memoized suppliers. Recognizing this, a third implementation is provided
     * that uses a Future along with a ReadWriteLock.
     *
     * @param <U>
     */
    public static class MemoizedSupplier<U> implements Supplier<U>, MemoizedCache {
        // TODO: share among MemoizedSuppliers to amortize the map overhead
        // over all memoized suppliers.
        private final ConcurrentMap<Supplier<U>, Computed<U>> cache = new ConcurrentHashMap<>();

        private final Supplier<U> s;
        private final Optional<Duration> expirationPeriod;

        /**
         * @param s
         */
        public MemoizedSupplier( Supplier<U> s ) {
            this( Optional.empty(), s );
        }

        /**
         * @param exp
         * @param s
         */
        public MemoizedSupplier( Optional<Duration> exp, Supplier<U> s ) {
            this.expirationPeriod = Objects.requireNonNull( exp );
            this.s = Objects.requireNonNull( s );
        }

        @Override
        public int size() {
            return cache.size();
        }

        @Override
        public void clear() {
            cache.clear();
        }

        @Override
        public void purgeExpired() {
            doPurgeExpired( expirationPeriod, cache );
        }

        public U get() {
            Computed<U> cu = null;
            while ( cu == null ) {
                cu = cache.computeIfAbsent( s, ( void_ ) -> {
                    return new Computed<U>( s.get() );
                } );
                if ( cu.isExpired( expirationPeriod ) ) {
                    // removing and recomputing should avoid the need for an
                    // external lock to be able to do this without the possibility of
                    // recomputing the value multiple times.
                    cache.remove( s );
                    cu = null;
                }
            }
            return cu.get();
        }
    }

    /**
     * Memoized Supplier implementation that use a cached Future to memoize
     * results. This implementation is based on Brian Goetz's Memoization
     * technique from Java Concurrency in Practice.
     *
     * @param <U>
     */
    public static class MemoizedSupplierFutureCache<U> implements Supplier<U>, MemoizedCache {
        // TODO: share to amortize added cost of map over all memoized
        // suppliers.
        private final ConcurrentMap<Supplier<U>, Future<Computed<U>>> cache = new ConcurrentHashMap<>();

        private final Supplier<U> s;
        private final Optional<Duration> expirationPeriod;

        public MemoizedSupplierFutureCache( Optional<Duration> expirationPeriod, Supplier<U> s ) {
            this.expirationPeriod = Objects.requireNonNull( expirationPeriod );
            this.s = Objects.requireNonNull( s );
        }

        @Override
        public int size() {
            return cache.size();
        }

        @Override
        public void clear() {
            cache.clear();
        }

        @Override
        public void purgeExpired() {
            doPurgeExpiredFuture(expirationPeriod, cache);
         }

        public U get() {
            try {
                Callable<Computed<U>> wrapper = () -> {
                    return new Computed<U>( s.get() );
                };
                return memoizedLookup( cache, s, expirationPeriod, wrapper );
            } catch ( InterruptedException ie ) {
                // reset the interrupted status
                Thread.currentThread().interrupt();

                throw new IllegalStateException( "Interrupted", ie );
            }
        }
    }

    /**
     * Alternative implementation that does not have the additional overhead of
     * the ConcurrentHashMap but uses a Future guarded by a read-write lock.
     *
     * This implementation supports an optional expiration period.
     */
    public static class MemoizedSupplierFuture<U> implements Supplier<U> {
        private final Supplier<U> s;
        private final Optional<Duration> expirationPeriod;

        private final ReadWriteLock rw = new ReentrantReadWriteLock();
        private final Lock r = rw.readLock();
        private final Lock w = rw.writeLock();

        private Future<Computed<U>> future;

        public MemoizedSupplierFuture( Optional<Duration> expirationPeriod, Supplier<U> s ) {
            this.s = s;
            this.expirationPeriod = expirationPeriod;
        }

        public U get() {
            while ( true ) {
                r.lock();
                try {
                    if ( future == null ) {
                        r.unlock();
                        w.lock();
                        try {
                            if ( future == null ) {
                                future = CompletableFuture.supplyAsync( () -> new Computed<U>( s.get() ) );
                            }
                        } finally {
                            r.lock();
                            w.unlock();
                        }
                    }

                    // at this point we know we have future that is either
                    // done (possibly cancelled) or in process. We can attempt
                    // to get (and return) the result. If canceled, we'll retry
                    // through CancellationException handling.
                    Computed<U> cu = future.get();

                    // check expiration. if expired, throw a
                    // CancellationException and allow its handling
                    // to reset the future and retry
                    if ( cu.isExpired( expirationPeriod ) ) {
                        throw new CancellationException( "expired" );
                    }

                    return cu.get();

                } catch ( CancellationException ce ) {

                    r.unlock();
                    w.lock();
                    try {
                        // just set to null and allow the retry handling to
                        // reset it.
                        future = null;
                    } finally {
                        r.lock();
                        w.unlock();
                    }

                    // retry loop
                    continue;

                } catch ( InterruptedException ie ) {
                    // reset the interrupted status
                    Thread.currentThread().interrupt();

                    // bail out -
                    throw new IllegalStateException( "Interrupted", ie );
                } catch ( ExecutionException ee ) {
                    try {
                        throw ee.getCause();
                    } catch ( RuntimeException re ) {
                        throw re;
                    } catch ( Error er ) {
                        throw er;
                    } catch ( Throwable th ) {
                        throw new IllegalStateException( "Unhandled Checked Exception", th );
                    }
                } finally {
                    r.unlock();
                }
            }
        }
    }

    /**
     * For all arguments <T> cache the computed result of the function <U>.
     * <p>
     * Important: this will not work for general recursive functions, such as
     * recursive implementations of fibonacci or factorial. In addition to
     * requiring that they recursively call the memoized version (See
     * https://dzone.com/articles/do-it-java-8-recursive-lambdas for some
     * discussion, the use of a ConcurrentHashMap can result in deadlock when
     * used with recursive functions.
     *
     * Usage:
     *
     * Function<Integer,Integer> inc = new MemoizedFunction.memoize((i) -> { ...
     * some long-running calculation ... return result; } );
     *
     * @param <T>
     * @param <U>
     */
    public static class MemoizedFunction<T, U> implements Function<T, U>, MemoizedCache {

        private final Function<T, U> f;
        private final Optional<Duration> expirationPeriod;

        // NOTE: use of ConcurrentHashMap here will result in deadlock
        // IF the function is recursive!
        private final ConcurrentMap<T, Computed<U>> cache = new ConcurrentHashMap<>();

        private MemoizedFunction( Function<T, U> f ) {
            this( Optional.empty(), f );
        }

        private MemoizedFunction( Optional<Duration> expiration, Function<T, U> f ) {
            this.f = f;
            this.expirationPeriod = expiration;
        }

        @Override
        public int size() {
            return cache.size();
        }

        @Override
        public void clear() {
            cache.clear();
        }

        @Override
        public void purgeExpired() {
            doPurgeExpired( expirationPeriod, cache );
        }


        @Override
        public U apply( T t ) {
            Computed<U> cu = null;
            while ( cu == null ) {
                cu = cache.computeIfAbsent( t, ( tt ) -> new Computed<>( f.apply( tt ) ) );
                if ( cu.isExpired( expirationPeriod ) ) {
                    // removing and recomputing
                    cache.remove( t );
                    cu = null;
                }
            }
            return cu.get();
        }
    }

    /**
     * A Memoized function that supports expiration using a Future to guarantee
     * one calculation per expiration period. This implementation is based on
     * Brian Goetz's Memoization technique from Java Concurrency in Practice.
     *
     * @param <T>
     * @param <U>
     */
    public static class MemoizedFunctionFuture<T, U> implements Function<T, U>, MemoizedCache {

        /**
         * @param function
         *            the function, which must not be recursive
         * @return the memoized function
         */
        public static <T, U> Function<T, U> memoize( final Function<T, U> function ) {
            return new MemoizedFunctionFuture<T, U>( Optional.empty(), function );
        }

        /**
         *
         * @param exp
         * @param function
         *            the function, which must not be recursive.
         * @return the memoized function
         */
        public static <T, U> Function<T, U> memoize( Optional<Duration> exp, final Function<T, U> function ) {
            return new MemoizedFunctionFuture<T, U>( exp, function );
        }

        private final Function<T, U> f;
        private final Optional<Duration> expirationPeriod;

        // NOTE: use of ConcurrentHashMap here will result in deadlock if the
        // function is recursive!
        private final ConcurrentMap<T, Future<Computed<U>>> cache = new ConcurrentHashMap<>();

        private MemoizedFunctionFuture( Optional<Duration> expiration, Function<T, U> f ) {
            this.f = f;
            this.expirationPeriod = expiration;
        }

        @Override
        public int size() {
            return cache.size();
        }

        @Override
        public void clear() {
            cache.clear();
        }

        @Override
        public void purgeExpired() {
            doPurgeExpiredFuture(expirationPeriod, cache);
        }

        @Override
        public U apply( T t ) {
            try {
                Callable<Computed<U>> wrapper = () -> {
                    return new Computed<U>( f.apply( t ) );
                };
                return memoizedLookup( cache, t, expirationPeriod, wrapper );
            } catch ( InterruptedException ie ) {
                // reset the interrupted status
                Thread.currentThread().interrupt();

                throw new IllegalStateException( "Interrupted", ie );
            }
        }
    }

    /**
     *
     * @param expirationPeriod
     * @param cache
     */
    protected static <T, U> void doPurgeExpiredFuture(Optional<Duration> expirationPeriod,
                                                      Map<T, Future<Computed<U>>> cache) {

        Iterator<Map.Entry<T, Future<Computed<U>>>> eiter = cache.entrySet().iterator();
        while ( eiter.hasNext() ) {
            Map.Entry<T, Future<Computed<U>>> e = eiter.next();
            Future<Computed<U>> fcu = e.getValue();
            if ( !fcu.isDone() )
                continue;

            try {
                // should never block based on previous isDone check.
                Computed<U> cu = fcu.get();
                if ( cu.isExpired( expirationPeriod ) ) {
                    eiter.remove();
                }
            } catch (Exception ex) {
                eiter.remove();
            }
        }
    }

    protected static <T,U> void doPurgeExpired(Optional<Duration> expirationPeriod,
                                               Map<T, Computed<U>> cache) {

        Iterator<Map.Entry<T, Computed<U>>> eiter = cache.entrySet().iterator();
        while ( eiter.hasNext() ) {
            Map.Entry<T, Computed<U>> e = eiter.next();
            if ( e.getValue().isExpired( expirationPeriod ) )
                eiter.remove();
        }
    }

    /**
     * Perform a non-expiring memoized lookup.
     *
     * @param cache
     * @param key
     * @param wrapper
     *
     * @return the computed value
     *
     * @throws InterruptedException
     */
    static <K, R> R memoizedLookup( ConcurrentMap<K, Future<Computed<R>>> cache,
                                    K key,
                                    Callable<Computed<R>> wrapper ) throws InterruptedException {
        return memoizedLookup( cache, key, Optional.empty(), wrapper );
    }

    /**
     * Given a ConcurrentMap that caches Future computed values, a cache key, an
     * expiration period, and a Callable that returns a Computed value, perform
     * a thread-safe memoized cache lookup for the computed value. If the value
     * is not present, or if it is expired, then the value is recomputed.
     *
     * @param cache
     * @param key
     * @param expirationPeriod
     *            the optional expiration period
     * @param wrapper
     *            a Callable wrapping an operation to compute a value
     *
     * @return the computed value
     *
     * @throws InterruptedException
     *             if the lookup and/or computation is interrupted.
     *
     * @param <K>
     *            the key type
     * @param <R>
     *            the result type
     */
    // TODO: add timeout?
    static <K, R> R memoizedLookup( ConcurrentMap<K, Future<Computed<R>>> cache,
                                    K key,
                                    Optional<Duration> expirationPeriod,
                                    Callable<Computed<R>> wrapper ) throws InterruptedException {

        // Note: this implementation is based on the thread-safe memoization
        // example from
        // Java Concurrency In Practice by Brian Goetz and related review at
        // http://www.javaspecialists.eu/archive/Issue125.html
        // with the addition of handling the expiration of the entry.
        Future<Computed<R>> fcu = null;
        while ( true ) {
            fcu = cache.get( key );
            if ( fcu == null ) {
                FutureTask<Computed<R>> ftcu = new FutureTask<>( wrapper );
                Future<Computed<R>> existing = cache.putIfAbsent( key, ftcu );
                if ( existing == null ) {
                    fcu = ftcu;
                    ftcu.run();
                } else {
                    // some other thread beat us to it. use the existing entry
                    // which can be assumed to have been run.
                    fcu = existing;
                }
            }

            try {
                Computed<R> cu = fcu.get();
                if ( cu.isExpired( expirationPeriod ) ) {
                    // treat like a CancellationException
                    throw new CancellationException( "Entry is expired" );
                }
                return cu.get();
            } catch ( CancellationException ce ) {
                cache.remove( key );

                // retry from the beginning
                continue;
            } catch ( ExecutionException ee ) {
                try {
                    throw ee.getCause();
                } catch ( RuntimeException re ) {
                    throw re;
                } catch ( Error er ) {
                    throw er;
                } catch ( Throwable th ) {
                    throw new IllegalStateException( "Unhandled CheckedException", th );
                }
            }
            // changed to throw InterruptedException
            // catch (InterruptedException ie) {
            // // reset the interrupted status and break out
            // Thread.currentThread().interrupt();
            //
            // // TODO: actually not clear what the response should be. perhaps
            // // an illegal state. Can't re-throw interrupted without changing
            // // signature. If we retry after resetting interrupt status, we'll
            // // get into an endless loop. If we don't reset and just retry,
            // then
            // // an external cancellation/interruption attempt (i.e. threadpool
            // executor
            // // shutdown request) will never be honored.
            // // returning null is likely to cause NPE down the road...
            // throw new IllegalStateException("Interrupted", ie);
            // }
        }
    }

}