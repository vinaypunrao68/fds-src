/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.crud;

import redis.clients.jedis.Jedis;

import java.io.Serializable;

/**
 * @author ptinius
 */
public abstract class KeyValueRepository<T, PrimaryKey extends Serializable>
  implements CRUDRepository<T, PrimaryKey> {

  private Jedis jedis;

  /**
   * @return Returns the {@link redis.clients.jedis.Jedis}
   */
  public Jedis redis() {
    return jedis;
  }

  /**
   * @param jedis the {@link redis.clients.jedis.Jedis}
   */
  public void redis( final Jedis jedis ) {
    this.jedis = jedis;
  }
}
