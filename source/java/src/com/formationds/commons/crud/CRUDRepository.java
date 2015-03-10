/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.crud;

import com.formationds.om.repository.query.QueryCriteria;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;

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
     * Persist the specified entities.
     * <p/>
     * This implementations converts the array of entities to a collection
     * and calls {@link #save(Collection)}
     *
     * @param entities the array of entities to persist
     *
     * @return the persisted events.
     *
     * @throws RuntimeException if the save for any entity fails
     */
    default List<T> save(final T... entities )     {
        return (entities != null ? save( Arrays.asList( entities )) : new ArrayList<>(0));
    }

    /**
     * Persist the specified entities.
     * <p/>
     * This implementation iterates over the set of entities and persists them
     * individually.  Sub-classes that support a bulk-save mechanism should
     * override this method to improve performance.
     *
     * @param entities the array of entities to persist
     *
     * @return the persisted events.
     *
     * @throws RuntimeException if the save for any entity fails
     */
    default List<T> save(final Collection<T> entities) {
        List<T> persisted = new ArrayList<>( );
        if (entities == null) {
            return persisted;
        }

        for (T e : entities) {
            persisted.add( save( e ) );
        }
        return persisted;
    }

    /**
     * @param queryCriteria the search criteria
     *
     * @return Returns the search criteria results
     */
    List<? extends T> query( final QueryCriteria queryCriteria );

    // TODO: delete( QueryCriteria )
    // TODO: update( T entity );
    // TODO: update( T model, QueryCriteria)
    /**
     * @param entity the entity to delete
     */
    void delete( final T entity );

    /**
     * @param entities the entities to delete
     */
    default void delete(final T... entities ) {
        if (entities != null ) {
            delete( Arrays.asList( entities ) );
        }
    }

    /**
     * Delete the specified entities.
     *
     * @param entities the array of entities to delete
     *
     * @throws RuntimeException if the delete for any entity fails
     */
    default void delete(final Collection<T> entities) {
        if (entities != null) {
            for ( T e : entities ) {
                delete( e );
            }
        }
    }

    /**
     * close the repository
     */
    void close();
}
