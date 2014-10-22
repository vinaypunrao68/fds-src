/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.cache;

/**
 * @author ptinius
 */
class CacheObject<V> {
  public V object;
  public LinkedListNode lastAccessedListNode;
  public LinkedListNode ageListNode;
  public int readCount = 0;

  /**
   * @param object the underlying Object to wrap.
   */
  public CacheObject( final V object ) {
    this.object = object;
  }

  /**
   * @param o the reference object with which to compare
   *
   * @return Returns {@code true} if this object is the same as the {@code o}
   * argument; {@code false} otherwise.
   */
  @Override
  public boolean equals( final Object o ) {
    if( this == o ) {
      return true;
    }
    if( !( o instanceof CacheObject ) ) {
      return false;
    }

    final CacheObject cacheObject = ( CacheObject ) o;

    return object.equals( cacheObject.object );

  }

  /**
   * @return Returns a hash code value for this {@link CacheObject}
   */
  @Override
  public int hashCode() {
    return object.hashCode();
  }
}
