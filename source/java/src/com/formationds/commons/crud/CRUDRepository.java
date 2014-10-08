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

import java.io.Serializable;
import java.util.List;
import java.util.Map;

/**
 * @author ptinius
 */
public interface CRUDRepository<T, PrimaryKey extends Serializable> {
  /**
   * @return the class
   */
  Class<T> getEntityClass();

  /**
   * @param primaryKey the primary key
   *
   * @return the entity
   */
  T findById( final PrimaryKey primaryKey );

  /**
   * @return the list of entities
   */
  List<T> findAll();

  /**
   * @param exampleInstance the example
   *
   * @return the list of entities
   */
  List<T> find( final T exampleInstance );

  /**
   * @param queryName the name of the query
   * @param params    the query parameters
   *
   * @return the list of entities
   */
  List<T> find( final String queryName, final Object... params );

  /**
   * @param queryName the name of the query
   * @param params    the query parameters
   *
   * @return the list of entities
   */
  List<T> find( final String queryName,
                final Map<String, ? extends Object> params );

  /**
   * @return the number of entities
   */
  long countAll();

  /**
   * @param entity the search criteria
   *
   * @return the number of entities
   */
  long countAllBy( final T entity );


  /**
   * @param entity the entity to save
   *
   * @return Returns the saved entity
   */
  T save( final T entity );

  /**
   * @param entity the entity to delete
   */
  void delete( final T entity );
}
