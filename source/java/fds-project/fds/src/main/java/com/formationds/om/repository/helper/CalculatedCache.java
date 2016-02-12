/*
 * Copyright (c) 2016, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.repository.helper;

import com.formationds.commons.model.abs.Calculated;
import com.google.common.base.Preconditions;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.time.Duration;
import java.time.Instant;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import java.util.function.Supplier;

/**
 * @author ptinius
 */
public enum CalculatedCache {
    instance;

    private static final Logger logger = LoggerFactory.getLogger( CalculatedCache.class );

    /**
     * Wrap a calculated value with a supplier (that calculates the value to
     * begin with) and a last updated timestamp to allow a time-based
     * recalculation. Also supports an optional expiration period that defaults
     * to never recalculate.
     *
     * @param <C>
     */
    public static class CalculatedValue<C extends Calculated> {
        //
        public static final Duration NEVER = Duration.ofMillis( Long.MAX_VALUE );
        public static final Duration HOURLY = Duration.ofHours( 1 );
        public static final Duration DAILY = Duration.ofDays( 1 );

        final String key;

        final Supplier<C> calculateValue;

        // ZERO always recompute.
        final Duration expirationPeriod;

        volatile C value;
        Instant lastUpdated = null;

        Lock updaterLock = new ReentrantLock();
        CompletableFuture<C> updater = null;

        public CalculatedValue( String key, Supplier<C> calculateValue ) {
            this( key, NEVER, calculateValue );
        }

        public CalculatedValue( String key, Duration expiration, Supplier<C> calculateValue ) {
            Preconditions.checkNotNull( key );
            Preconditions.checkNotNull( expiration );
            Preconditions.checkArgument( !expiration.isNegative() );
            Preconditions.checkNotNull( calculateValue );

            this.key = key;
            this.calculateValue = calculateValue;
            this.expirationPeriod = expiration;
            this.value = null;
        }

        /**
         * @return the key
         */
        public String getKey() {
            return key;
        }

        /**
         *
         * @return the current calculated value
         *
         * @throws InterruptedException
         *             if the request for the value is interrupted before the
         *             value is available
         */
        public C getValue() throws InterruptedException {
            C val = value;
            if ( val == null || isExpired() ) {
                try {
                    recalcuateAndSet().get();
                    val = value;
                } catch ( ExecutionException e ) {
                    throw new IllegalStateException( "Failed to calculate cached value", e.getCause() );
                }
            }
            return val;
        }

        /**
         *
         * @return the current calculated value
         *
         * @throws InterruptedException
         *             if the request for the value is interrupted before the
         *             value is available
         */
        public C getValue( long timeout, TimeUnit unit ) throws InterruptedException, TimeoutException {
            C val = value;
            if ( val == null ) {
                try {
                    recalcuateAndSet().get( timeout, unit );
                    val = value;
                } catch ( ExecutionException e ) {
                    throw new IllegalStateException( "Failed to calculate cached value", e.getCause() );
                }
            } else if ( isExpired() ) {
                // return current non-null value but trigger asynchronous update
                recalcuateAndSet();
            }
            return val;
        }

        /**
         * @return true if this value is expired
         */
        public boolean isExpired() {
            return lastUpdated == null ||
                    Duration.between( lastUpdated, Instant.now() ).compareTo( expirationPeriod ) > 0;
        }

        /**
         *
         * @param defaultValue
         * @return
         */
        public C getValueNow( C defaultValue ) {
            C v = value;
            if ( v == null || isExpired()) {
                if ( isExpired() ) {
                    // trigger calculation
                    recalcuateAndSet();
                }
                return defaultValue;
            }
            return v;
        }

        /**
         * Recalculate the specified value on a separate thread
         */
        private CompletableFuture<C> recalculate() {
            updaterLock.lock();
            try {
                if ( updater == null )
                    updater = CompletableFuture.supplyAsync( calculateValue );
                return updater;
            } finally {
                updaterLock.unlock();
            }
        }

        private CompletableFuture<Void> recalcuateAndSet() {
            return recalculate().thenAccept( this::setValue );
        }

        private synchronized void setValue( C value ) {
            this.value = value;
            lastUpdated = Instant.now();

            updaterLock.lock();
            try {
                updater = null;
            } finally {
                updaterLock.unlock();
            }
        }

        /**
         * @return the lastUpdated timesatamp
         */
        public Instant getLastUpdated() {
            return lastUpdated;
        }

        /*
         * (non-Javadoc)
         *
         * @see java.lang.Object#hashCode()
         */
        @Override
        public int hashCode() {
            return Objects.hash( key );
        }

        /*
         * (non-Javadoc)
         *
         * @see java.lang.Object#equals(java.lang.Object)
         */
        @Override
        public boolean equals( Object obj ) {
            if ( this == obj )
                return true;
            if ( obj == null )
                return false;
            if ( getClass() != obj.getClass() )
                return false;
            CalculatedValue<?> other = (CalculatedValue<?>) obj;
            return Objects.equals( key, other.key );
        }
    }

    private final ConcurrentMap<String, CalculatedValue<? extends Calculated>> calculatedCache = new ConcurrentHashMap<>();

    public <C extends Calculated> void registerCalculation( String key, Duration expiration, Supplier<C> supplier ) {
        CalculatedValue<C> wrapper = new CalculatedValue<>( key, expiration, supplier );
        calculatedCache.putIfAbsent( key, wrapper );
    }

    public Set<String> registeredCalculationKeys() { return calculatedCache.keySet(); }
    public boolean isRegistered(String key) { return calculatedCache.containsKey( key ); }

    @SuppressWarnings("unchecked")
    public <C extends Calculated> C getValue( String key ) throws InterruptedException {
        return (C) calculatedCache.get( key ).getValue();
    }
}
