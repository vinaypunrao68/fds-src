/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util;

import org.apache.commons.lang3.StringUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Iterator;

/**
 * @author ptinius
 */
public class Numbers<E>
  extends ArrayList<E> {
  private static final long serialVersionUID = 8301460928201665544L;

  private NumbersValidator<E> validator;

  /**
   * Constructs an empty list with an initial capacity of ten.
   */
  public Numbers() {
  }

  /**
   * Constructs an empty list with the specified initial capacity.
   *
   * @param initialCapacity the initial capacity of the list
   *
   * @throws IllegalArgumentException if the specified initial capacity is
   *                                  negative
   */
  public Numbers( final int initialCapacity ) {
    super( initialCapacity );
  }

  /**
   * Constructs a list containing the elements of the specified collection, in
   * the order they are returned by the collection's iterator.
   *
   * @param c the collection whose elements are to be placed into this list
   *
   * @throws NullPointerException if the specified collection is null
   */
  public Numbers( final Collection<? extends E> c ) {
    super( c );
  }

  /**
   * @param validator the {@link com.formationds.commons.util.NumbersValidator}
   */
  public void validator( final NumbersValidator<E> validator ) {
    this.validator = validator;
  }

  /**
   * @return Returns the {@link com.formationds.commons.util.NumbersValidator}
   */
  public NumbersValidator<E> validator() {
    return validator;
  }

  /**
   * Appends the specified element to the end of this list.
   *
   * @param e element to be appended to this list
   *
   * @return <tt>true</tt> (as specified by {@link java.util.Collection#add})
   */
  @Override
  public boolean add( final E e ) {

    return super.add( e );
  }

  /**
   * @param separated [the {@link String} representing the values delimited by
   *                  {@code delimiter}
   * @param delimiter the {@link String} delimiter
   *
   * @return Returns {@code true} if {@link Numbers} was updated. Otherwise
   * {@code false}
   */
  @SuppressWarnings( "unchecked" )
  public boolean add( final String separated, final String delimiter ) {
    return addAll( Arrays.asList( ( E[] ) StringUtils.split( separated,
                                                             delimiter ) ) );
  }

  /**
   * @return Returns {@link String} representing the {@link Numbers} object.
   */
  public final String toString() {
    final StringBuilder b = new StringBuilder();

    for( final Iterator<E> i = iterator();
         i.hasNext(); ) {
      b.append( i.next() );
      if( i.hasNext() ) {
        b.append( ',' );
      }
    }

    return b.toString();
  }
}
