/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.tools.statgen;

import java.security.SecureRandom;
import java.util.Random;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

/**
 * Provides various StatValueGenerator implementations for different numeric types
 * and constant, sequential, random, and additive generators.
 * <p/>
 * Factory methods are provided for easy api access to the various types of generators.
 */
public class StatValueGenerators {

    public static final Random RNG = new SecureRandom();

    public static StatValueGenerator<Integer> sequentialInt() { return new SequentialSVGI(); }

    public static StatValueGenerator<Integer> sequentialInt( int start ) { return new SequentialSVGI( start ); }

    public static StatValueGenerator<Long> sequentialLong() { return new SequentialSVGL(); }

    public static StatValueGenerator<Long> sequentialLong( long start ) { return new SequentialSVGL( start ); }

    public static StatValueGenerator<Number> constant( Number c ) { return new ConstantSVG<>( c ); }

    public static StatValueGenerator<Integer> randomInt() { return new RndSVGI(); }

    public static StatValueGenerator<Long> randomLong() { return new RndSVGL(); }

    public static StatValueGenerator<Double> randomDouble() { return new RndSVGD(); }

    public static StatValueGenerator<Integer> addInt() { return new AdditiveSVGI(); }

    public static StatValueGenerator<Integer> addInt( int min ) {
        return new AdditiveSVGI( min, Integer.MAX_VALUE, 1 );
    }

    public static StatValueGenerator<Integer> addInt( int min,
                                                      int inc ) {
        return new AdditiveSVGI( min,
                                 Integer.MAX_VALUE,
                                 inc );
    }

    public static StatValueGenerator<Integer> addInt( int min, int max, int inc ) {
        return new AdditiveSVGI( min,
                                 max,
                                 inc );
    }

    public static StatValueGenerator<Long> addLong() { return new AdditiveSVGL(); }

    public static StatValueGenerator<Long> addLong( long min,
                                                    int inc ) {
        return new AdditiveSVGL( min,
                                 Integer.MAX_VALUE,
                                 inc );
    }

    public static StatValueGenerator<Long> addLong( long min,
                                                    long max,
                                                    int inc ) {
        return new AdditiveSVGL( min,
                                 max,
                                 inc );
    }

    public static StatValueGenerator<Double> addDouble() { return new AdditiveSVGD(); }

    public static StatValueGenerator<Double> addDouble( double min,
                                                        float inc ) {
        return new AdditiveSVGD( min,
                                 Integer.MAX_VALUE,
                                 inc );
    }

    public static StatValueGenerator<Double> addDouble( double min,
                                                        double max,
                                                        float inc ) {
        return new AdditiveSVGD( min,
                                 max,
                                 inc );
    }

    public static FirebreakGenerator firebreak() { return new FirebreakGenerator(); }

    /**
     * Base class for generating a number Bound by a min and optional max value
     *
     * @param <N>
     */
    public static abstract class BoundedGenerator<N extends Number> implements StatValueGenerator<N> {
        protected N min;
        protected N max;

        BoundedGenerator( N min, N max ) {
            if ( min == null || min.longValue() < 0 ) {
                throw new IllegalArgumentException( "Min value must be >= 0" );
            }
            if ( max == null || max.longValue() < 0 ) {
                throw new IllegalArgumentException( "Max value must be >= 0" );
            }
            if ( min.longValue() >= max.longValue() ) {
                throw new IllegalArgumentException( "Min value must be less than the max" );
            }

            this.min = min;
            this.max = max;
        }

        public N generate() { return generate( min, max ); }

        abstract protected N generate( N min, N max );
    }

    /**
     * @param <N> StatValueGenerator that returns a constant value
     */
    public static class ConstantSVG<N extends Number> implements StatValueGenerator<N> {
        final N value;

        public ConstantSVG( N value ) { this.value = value; }

        public N generate() { return value; }
    }

    /**
     * Generator for random long values
     */
    public static class RndSVGL extends BoundedGenerator<Long> {
        public RndSVGL() {
            this( 0L, Long.MAX_VALUE );
        }

        public RndSVGL( Long min, Long max ) {
            super( min, max );
        }

        protected Long generate( Long min, Long max ) {
            if ( min == null ) { min = 0L; }
            if ( max == null ) { max = Long.MAX_VALUE; }

            if ( min < 0 ) {
                throw new IllegalArgumentException( "Min value must be >= 0" );
            }
            if ( min >= max ) {
                throw new IllegalArgumentException( "Min value must be less than the max" );
            }

            if ( (max - min) < Integer.MAX_VALUE ) {
                return min + RNG.nextInt( (int) (max - min) );
            } else {
                long r = min;
                while ( (r = RNG.nextLong()) < min ) {
                    long mc = min + r;

                    // overflow?
                    if ( mc < min ) { continue; }

                    if ( mc < max ) { return min + r; }
                }
                return r;
            }
        }
    }

    /**
     * Generator for random integer values
     */
    public static class RndSVGI extends BoundedGenerator<Integer> {
        public RndSVGI() {
            this( 0, Integer.MAX_VALUE );
        }

        public RndSVGI( Integer min, Integer max ) {
            super( min, max );
        }

        protected Integer generate( Integer min, Integer max ) {
            if ( min == null ) { min = 0; }
            if ( max == null ) { max = Integer.MAX_VALUE; }

            if ( min < 0 ) {
                throw new IllegalArgumentException( "Min value must be >= 0" );
            }
            if ( min >= max ) {
                throw new IllegalArgumentException( "Min value must be less than the max" );
            }

            return min + RNG.nextInt( max - min );
        }
    }

    /**
     * Generator for random double values.
     * <p/>
     * Random.nextDouble produces a value between 0 and 1.0.  This operates
     * by multiply7ing the random by the max value..
     */
    public static class RndSVGD extends BoundedGenerator<Double> {
        private static final RndSVGD instance = new RndSVGD();

        public static Double genDouble() {
            return instance.generate();
        }

        public static Double genDouble( Double min, Double max ) {
            return instance.generate( min, max );
        }

        public RndSVGD() {
            this( 0D, Double.MAX_VALUE );
        }

        public RndSVGD( Double min, Double max ) {
            super( min, max );
        }

        public Double generate( Double min, Double max ) {
            if ( min == null ) { min = 0D; }
            if ( max == null ) { max = Double.MAX_VALUE; }

            if ( min < 0 ) {
                throw new IllegalArgumentException( "Min value must be >= 0" );
            }
            if ( min >= max ) {
                throw new IllegalArgumentException( "Min value must be less than the max" );
            }

            double r = min;
            while ( Double.compare( (r = (RNG.nextDouble() * max)), min ) < 0 ) {
                double mc = Double.sum( min, r );

                // overflow?
                if ( Double.compare( mc, min ) < 0 ) { continue; }

                if ( Double.compare( mc, max ) < 0 ) { return Double.sum( min, r ); }
            }
            return r;
        }
    }

    public static class SequentialSVGL implements StatValueGenerator<Long> {
        private AtomicLong seq = new AtomicLong();

        public SequentialSVGL() {
        }

        public SequentialSVGL( long start ) {
            seq.set( start );
        }

        @Override
        public Long generate() {
            return seq.incrementAndGet();
        }

        public Long generate( Long min, Long max ) {
            if ( min == null ) { min = 0L; }
            if ( max == null ) { max = Long.MAX_VALUE; }
            long v = seq.incrementAndGet();
            if ( Math.min( min, v ) == v ) { return min; } else if ( Math.max( max, v ) == v ) { return max; }
            return v;
        }
    }

    public static class SequentialSVGI implements StatValueGenerator<Integer> {
        private AtomicInteger seq = new AtomicInteger();

        public SequentialSVGI() {
        }

        public SequentialSVGI( int start ) {
            seq.set( start );
        }

        @Override
        public Integer generate() {
            return seq.incrementAndGet();
        }

        public Integer generate( Integer min, Integer max ) {
            if ( min == null ) { min = 0; }
            if ( max == null ) { max = Integer.MAX_VALUE; }
            int v = seq.incrementAndGet();
            if ( Math.min( min, v ) == v ) { return min; } else if ( Math.max( max, v ) == v ) { return max; }
            return v;
        }
    }

    public static class AdditiveSVGL extends BoundedGenerator<Long> {
        private AtomicLong seq       = new AtomicLong();
        private int        increment = 1;

        public AdditiveSVGL() {
            this( 0L, Long.MAX_VALUE, 1 );
        }

        public AdditiveSVGL( long start, long max, int increment ) {
            super( start, max );
            seq.set( start );
            this.increment = increment;
        }

        @Override
        protected Long generate( Long min, Long max ) {
            if ( min == null ) { min = 0L; }
            if ( max == null ) { max = Long.MAX_VALUE; }

            // todo: handle overflow
            long v = seq.addAndGet( increment );
            if ( Math.min( min, v ) == v ) { return min; } else if ( Math.max( max, v ) == v ) { return max; }
            return v;
        }
    }

    public static class AdditiveSVGI extends BoundedGenerator<Integer> {
        private AtomicInteger seq       = new AtomicInteger();
        private int           increment = 1;

        public AdditiveSVGI() {
            this( 0, Integer.MAX_VALUE, 1 );
        }

        public AdditiveSVGI( int start, int max, int increment ) {
            super( start, max );
            seq.set( start );
            this.increment = increment;
        }

        @Override
        public Integer generate() {
            return seq.addAndGet( increment );
        }

        protected Integer generate( Integer min, Integer max ) {
            if ( min == null ) { min = 0; }
            if ( max == null ) { max = Integer.MAX_VALUE; }

            // todo handle int overflow
            int v = seq.addAndGet( increment );
            if ( Math.min( min, v ) == v ) { return min; } else if ( Math.max( max, v ) == v ) { return max; }
            return v;
        }
    }

    public static class AdditiveSVGD extends BoundedGenerator<Double> {
        private double seq       = 0.0D;
        private float  increment = .01F;

        public AdditiveSVGD() {
            this( 0, Double.MAX_VALUE, .01F );
        }

        public AdditiveSVGD( double start, float increment ) {
            this( start, Double.MAX_VALUE, increment );
        }

        public AdditiveSVGD( double start, double max, float increment ) {
            super( start, max );
            seq = start;
            this.increment = increment;
        }

        @Override
        protected Double generate( Double min, Double max ) {
            if ( min == null ) { min = 0D; }
            if ( max == null ) { max = Double.MAX_VALUE; }

            // todo: handle overflow
            double v = (seq += increment);
            if ( Math.min( min, v ) == v ) { return min; } else if ( Math.max( max, v ) == v ) { return max; }
            return v;
        }
    }
}
