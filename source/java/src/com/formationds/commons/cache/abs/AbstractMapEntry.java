/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.cache.abs;

import java.util.Map;

/**
 * @author ptinius
 */
public abstract class AbstractMapEntry<K, V>
  extends AbstractKeyValue<K, V>
  implements Map.Entry<K, V> {

  /**
   * @param key   the key for the entry, may be null
   * @param value the value for the entry, may be null
   */
  protected AbstractMapEntry( final K key, final V value ) {
    super( key, value );
  }

  // Map.Entry interface
  //-------------------------------------------------------------------------

  /**
   * @param value the new value
   *
   * @return Returns the previous value
   */
  public V setValue( final V value ) {
    V answer = this.value;
    this.value = value;
    return answer;
  }

  /**
   * @param obj the object to compare to
   *
   * @return Returns {@code true} if and only if equal key and value
   */
  public boolean equals( Object obj ) {
    if( obj == this ) {
      return true;
    }
    if( !( obj instanceof Map.Entry ) ) {
      return false;
    }
    Map.Entry other = ( Map.Entry ) obj;
    return ( getKey() == null ? other.getKey() == null : getKey().equals(
      other.getKey() ) ) &&
      ( getValue() == null ? other.getValue() == null : getValue().equals(
        other.getValue() ) );
  }

  /**
   * @return Returns a suitable hash code
   */
  public int hashCode() {
    return ( getKey() == null ? 0 : getKey().hashCode() ) ^
      ( getValue() == null ? 0 : getValue().hashCode() );
  }

}
