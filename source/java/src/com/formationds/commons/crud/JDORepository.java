/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.crud;

import javax.jdo.PersistenceManager;
import javax.jdo.PersistenceManagerFactory;
import java.io.Serializable;
import java.lang.reflect.ParameterizedType;

/**
 * @author ptinius
 */
public abstract class JDORepository<T, PrimaryKey extends Serializable>
  implements CRUDRepository<T, PrimaryKey> {
  private PersistenceManagerFactory factory;
  private PersistenceManager manager;

  /**
   * @return Returns the {@link PersistenceManagerFactory}
   */
  protected PersistenceManagerFactory factory() {
    return factory;
  }

  /**
   * @param factory the {@link PersistenceManagerFactory}
   */
  protected void factory( final PersistenceManagerFactory factory ) {
    this.factory = factory;
  }

  /**
   * @return Returns the {@link PersistenceManager}
   */
  protected PersistenceManager manager() {
    return manager;
  }

  /**
   * @param manager the {@link PersistenceManager}
   */
  protected void manager( final PersistenceManager manager ) {
    this.manager = manager;
  }

  /**
   * @return the class
   */
  @SuppressWarnings( "unchecked" )
  @Override
  public Class getEntityClass() {
    return ( Class<T> ) ( ( ParameterizedType ) getClass()
      .getGenericSuperclass( ) ).getActualTypeArguments( )[ 0 ];
  }

  /**
   * @param dbName the {@link String} representing the name and location of the repository
   */
  protected abstract void initialize( final String dbName );
}
