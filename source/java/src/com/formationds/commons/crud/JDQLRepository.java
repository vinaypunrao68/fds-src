/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.crud;

import javax.persistence.EntityManager;
import javax.persistence.EntityManagerFactory;
import java.io.Serializable;
import java.lang.reflect.ParameterizedType;

/**
 * @author ptinius
 */
public abstract class JDQLRepository<T,
  PrimaryKey extends Serializable,
  QueryResults extends SearchResults,
  QueryCriteria extends SearchCriteria>
  implements CRUDRepository<T, PrimaryKey, QueryResults, QueryCriteria> {

  private EntityManagerFactory factory;
  private EntityManager manager;
  private EntityManager entity;

  /**
   * @return Returns the {@link javax.persistence.EntityManagerFactory}
   */
  protected EntityManagerFactory factory() {
    return factory;
  }

  /**
   * @param factory the {@link javax.persistence.EntityManagerFactory}
   */
  protected void factory( final EntityManagerFactory factory ) {
    this.factory = factory;
  }

  /**
   * @return Returns the {@link javax.persistence.EntityManager}
   */
  protected EntityManager manager() {
    return manager;
  }

  /**
   * @param manager the {@link javax.persistence.EntityManager}
   */
  protected void manager( final EntityManager manager ) {
    this.manager = manager;
  }

  /**
   * @return Returns the {@link EntityManager}
   */
  public EntityManager entity() {
    return entity;
  }

  /**
   * @param entity the {@link javax.persistence.EntityManager}
   */
  protected void entity( final EntityManager entity ) {
    this.entity = entity;
  }

  /**
   * @return the class
   */
  @SuppressWarnings("unchecked")
  @Override
  public Class getEntityClass() {
    return ( Class<T> ) ( ( ParameterizedType ) getClass()
      .getGenericSuperclass() ).getActualTypeArguments()[ 0 ];
  }

  /**
   * @param dbName the {@link String} representing the name and location of the
   *               repository
   */
  protected abstract void initialize( final String dbName );
}
