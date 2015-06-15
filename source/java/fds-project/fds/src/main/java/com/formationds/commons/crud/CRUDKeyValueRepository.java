/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.crud;

import java.io.Serializable;

/**
 * @author ptinius
 */
public interface CRUDKeyValueRepository<K, V>
  extends Cloneable, Serializable {

  /**
   * @param key   the {@link K} representing the key
   * @param value the {@link V} representing the value
   *
   * @return Returns {@link V} representing the value
   */
  String set( final K key, final V value );

  /**
   * @param key   the {@link K} representing the key
   * @param value the {@link V} representing the value
   *
   * @return Returns {@link V} representing the value
   */
  String set( final K key, final V value, final long time );

  /**
   * @param key the {@link K} representing the key
   *
   * @return Returns {@link V} representing the value
   */
  V get( final K key );

  /**
   * @param key the {@link K} representing the key
   *
   * @return Returns {@link V} representing the value
   */
  V delete( final K key );

  /**
   * @param key the {@link K} representing the key
   *
   * @return Returns {@code true} if and only if the {@link K} exists with the
   * store
   */
  boolean containsKey( final K key );

  /**
   * @param value the {@link V} representing the value
   *
   * @return Returns {@code true} if and only if the {@link V} exists with the
   * store
   */
  boolean containsValue( final V value );

  /**
   * @return Returns {@code int} representing the size of the store
   */
  int size();

  /**
   * closes repository
   */
  void close();
}
