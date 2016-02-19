/*
 * Copyright (c) 2016, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.commons.util;

import java.util.Arrays;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Objects;

/**
 * Contains various Tuple classes that can be used in conjunction with
 * {@link java.util.function.Function} etc. to represent multiple arguments
 * passed to a function, and further enabling the use of {@link Memoizer} with
 * those functions.
 *
 * It provides a UnaryTuple, which is relatively useless for the purpose of
 * interaction with Function given that it is no different than a standard
 * Function. Similarly, a Pair is equivalent to a BiFunction. However, at least
 * with a Pair instead of a BiFunction, you can Memoize the function.
 *
 * Type-safe tuples provided include:
 *
 * <ul>
 * <li>UnaryTuple</li>
 * <li>Pair</li>
 * <li>Triple</li>
 * <li>Tuple4</li>
 * <li>Tuple5</li>
 * </ul>
 *
 * All of the type-safe tuples extend NTuple which provides an arbitrary-length
 * tuple of Objects.  NamedTuple provides an implementation with named value pairs.
 *
 */
public class Tuples {

    /**
     *
     * @param n
     * @param v
     *
     * @return
     */
    public static <T> Pair<String, T> namedPair( String n, T v ) {
        return new Pair<>( n, v );
    }

    /**
     *
     * @param pairs
     * @return
     */
    @SuppressWarnings("unchecked")
    public static NamedTuple namedTuple( Pair<String, ?>... pairs ) {
        return new NamedTuple( pairs );
    }

    /**
     *
     * @param objects
     * @return
     */
    @SuppressWarnings("unchecked")
    public static <N extends NTuple> N nTuple( Object... objects ) {
        switch ( objects.length ) {
            case 1:
                return (N) new UnaryTuple<Object>( objects[0] );
            case 2:
                return (N) new Pair<Object, Object>( objects[0], objects[1] );
            case 3:
                return (N) new Triple<Object, Object, Object>( objects[0], objects[1], objects[2] );
            case 4:
                return (N) new Tuple4<Object, Object, Object, Object>( objects[0], objects[1], objects[2], objects[3] );
            case 5:
                return (N) new Tuple5<Object, Object, Object, Object, Object>( objects[0], objects[1], objects[2],
                                                                               objects[3], objects[4] );
            default:
                // todo: if length is even and first and every other element is
                // a string, build
                // as a NamedTuple
                return (N) new NTuple( objects );
        }
    }

    public static <A> UnaryTuple<A> tupleOf( A a ) {
        return new UnaryTuple<>( a );
    }

    public static <A, B> Pair<A, B> tupleOf( A a, B b ) {
        return new Pair<>( a, b );
    }

    public static <A, B, C> Triple<A, B, C> tupleOf( A a, B b, C c ) {
        return new Triple<>( a, b, c );
    }

    public static <A, B, C, D> Tuple4<A, B, C, D> tupleOf( A a, B b, C c, D d ) {
        return new Tuple4<A, B, C, D>( a, b, c, d );
    }

    public static <A, B, C, D, E> Tuple5<A, B, C, D, E> tupleOf( A a, B b, C c, D d, E e ) {
        return new Tuple5<A, B, C, D, E>( a, b, c, d, e );
    }

    /**
     * Represent a Tuple of arbitrary size
     *
     */
    public static class NTuple implements Iterable<Object> {
        private final Object[] t;

        public NTuple( Object... objects ) {
            if ( objects == null )
                t = new Object[0];
            else
                this.t = Arrays.copyOf( objects, objects.length );
        }

        public Object[] getValues() {
            return Arrays.copyOf( t, t.length );
        }

        protected Object get( int i ) {
            return t[i];
        }

        public boolean equals( Object o ) {
            if ( !( o instanceof NTuple ) )
                return false;
            NTuple other = (NTuple) o;
            return Objects.deepEquals( this.t, other.t );
        }

        public int hashCode() {
            return Objects.hash( t );
        }

        @Override
        public Iterator<Object> iterator() {
            return Arrays.asList( t ).iterator();
        }
    }

    /**
     * Represent a Tuple of 1.
     *
     * @param <T>
     */
    public static class UnaryTuple<T> extends NTuple {
        public UnaryTuple( T t ) {
            super( t );
        }

        @SuppressWarnings("unchecked")
        public T get() {
            return (T) super.get( 0 );
        }
    }

    /**
     * Represent a pair or Tuple of 2 elements
     *
     * @param <T>
     * @param <T2>
     */
    public static class Pair<T, T2> extends NTuple {
        public Pair( T t, T2 t2 ) {
            super( t, t2 );
        }

        @SuppressWarnings("unchecked")
        public T getLeft() {
            return (T) super.get( 0 );
        }

        @SuppressWarnings("unchecked")
        public T2 getRight() {
            return (T2) super.get( 1 );
        }
    }

    /**
     * Represent a triple, or a tuple of 3 elements
     *
     * @param <T1>
     * @param <T2>
     * @param <T3>
     */
    public static class Triple<T1, T2, T3> extends NTuple {
        public Triple( T1 t1, T2 t2, T3 t3 ) {
            super( t1, t2, t3 );
        }

        @SuppressWarnings("unchecked")
        public T1 getLeft() {
            return (T1) super.get( 0 );
        }

        @SuppressWarnings("unchecked")
        public T2 getMiddle() {
            return (T2) super.get( 1 );
        }

        @SuppressWarnings("unchecked")
        public T3 getRight() {
            return (T3) super.get( 2 );
        }
    }

    @SuppressWarnings("unchecked")
    public static class NamedTuple extends NTuple {
        private Map<String, Integer> index = new HashMap<>();

        public NamedTuple( Pair<String, ?>... nameValuePairs ) {
            super( Objects.requireNonNull( (Object[]) nameValuePairs ) );
            for ( int i = 0; i < nameValuePairs.length; i++ ) {
                Pair<String, ?> p = nameValuePairs[i];
                index.put( p.getLeft(), i );
            }
        }

        public <T> T get( String name ) {
            int i = index.get( name );
            return (T) ( (Pair<String, ?>) super.get( i ) ).getRight();
        }
    }

    /**
     * Represent a tuple of 4 elements
     *
     * @param <A>
     * @param <B>
     * @param <C>
     * @param <D>
     */
    @SuppressWarnings("unchecked")
    public static class Tuple4<A, B, C, D> extends NTuple {

        public Tuple4( A a, B b, C c, D d ) {
            super( a, b, c, d );
        }

        public A getFirst() {
            return (A) super.get( 0 );
        }

        public B getSecond() {
            return (B) super.get( 1 );
        }

        public C getThird() {
            return (C) super.get( 2 );
        }

        public D getFourth() {
            return (D) super.get( 3 );
        }

    }

    /**
     * Represent a tuple of 5 elements
     *
     * @param <A>
     * @param <B>
     * @param <C>
     * @param <D>
     * @param <E>
     */
    @SuppressWarnings("unchecked")
    public static class Tuple5<A, B, C, D, E> extends NTuple {

        public Tuple5( A a, B b, C c, D d, E e ) {
            super( a, b, c, d, e );
        }

        public A getFirst() {
            return (A) super.get( 0 );
        }

        public B getSecond() {
            return (B) super.get( 1 );
        }

        public C getThird() {
            return (C) super.get( 2 );
        }

        public D getFourth() {
            return (D) super.get( 3 );
        }

        public E getFifth() {
            return (E) super.get( 4 );
        }

    }
}
