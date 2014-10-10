/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 *
 * This software is furnished under a license and may be used and copied only
 * in  accordance  with  the  terms  of such  license and with the inclusion
 * of the above copyright notice. This software or  any  other copies thereof
 * may not be provided or otherwise made available to any other person.
 * No title to and ownership of  the  software  is  hereby transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Formation Data Systems.
 *
 * Formation Data Systems assumes no responsibility for the use or  reliability
 * of its software on equipment which is not supplied by Formation Date Systems.
 */

package com.formationds.commons.crud;

import javax.persistence.EntityManager;
import javax.persistence.EntityManagerFactory;
import java.io.Serializable;
import java.lang.reflect.ParameterizedType;

/**
 * @author ptinius
 */
public abstract class JDQLRepository<T, PrimaryKey extends Serializable>
  implements CRUDRepository<T, PrimaryKey> {

  private EntityManagerFactory factory;
  private EntityManager manager;

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
