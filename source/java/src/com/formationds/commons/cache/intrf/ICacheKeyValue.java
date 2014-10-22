/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.cache.intrf;

/**
 * @author ptinius
 */
public interface ICacheKeyValue<K, V> {
  /**
   * @return the key
   */
  K getKey();

  /**
   * @return the value
   */
  V getValue();
}
