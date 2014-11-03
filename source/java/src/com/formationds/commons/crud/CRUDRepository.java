/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.crud;

import java.io.Serializable;
import java.util.List;

/**
 * @author ptinius
 */
public interface CRUDRepository<T,
  PrimaryKey extends Serializable,
  QueryResults extends SearchResults,
  QueryCriteria extends SearchCriteria> {

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
   * @param queryCriteria the search criteria
   *
   * @return Returns the search criteria results
   */
  QueryResults query( final QueryCriteria queryCriteria );

  /**
   * @param entity the entity to delete
   */
  void delete( final T entity );

  /**
   * close the repository
   */
  void close();
}
