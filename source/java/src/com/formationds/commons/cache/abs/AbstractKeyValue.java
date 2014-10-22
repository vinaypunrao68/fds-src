/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.cache.abs;

import com.formationds.commons.cache.intrf.ICacheKeyValue;

/**
 * @author ptinis
 */
public abstract class AbstractKeyValue<K, V>
  implements ICacheKeyValue<K, V> {

  /**
   * The key
   */
  protected K key;
  /**
   * The value
   */
  protected V value;

  /**
   * @param key   the key for the entry, may be null
   * @param value the value for the entry, may be null
   */
  protected AbstractKeyValue( final K key, final V value ) {
    super();
    this.key = key;
    this.value = value;
  }

  /**
   * @return Returns the key
   */
  public K getKey() {
    return key;
  }

  /**
   * @return Returns the value
   */
  public V getValue() {
    return value;
  }

  /**
   * @return Returns a string representation of the entry.
   */
  @Override
  public String toString() {
    return getKey() + " = " + getValue();
  }
}
