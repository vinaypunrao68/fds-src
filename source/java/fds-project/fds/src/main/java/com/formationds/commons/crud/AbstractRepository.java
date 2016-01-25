/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.commons.crud;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.Serializable;
import java.lang.reflect.ParameterizedType;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Properties;

/**
 * @param <T>  the entity type managed by this repository
 * @param <PK> the type of the primary key identifier for an entity stored in the repository
 */
abstract public class AbstractRepository<T, PK extends Serializable> implements CRUDRepository<T, PK> {
    private final List<EntityPersistListener<T>> listeners = new ArrayList<>();

    // use non-static logger to tie it to the concrete subclass.
    protected final Logger logger;

    /**
     *
     */
    protected AbstractRepository() {
        logger = LoggerFactory.getLogger( this.getClass() );
    }

    /**
     * Open the repository and initialize any connection/entity management necessary for
     * interacting with the underlying repository.
     */
    abstract public void open(Properties properties);

    /**
     * No-op implementation of close.  Override in sub-class if it is necessary to
     * perform any specific resource cleanup on close.
     */
    public void close() {}

    /**
     * Add a Entity persist listener for pre/post persistence callbacks
     *
     * @param l the listener
     */
    public void addEntityPersistListener( EntityPersistListener<T> l ) {
        listeners.add( l );
    }

    /**
     * Remove the entity persist listener.
     *
     * @param l the listener to remove
     */
    public void removeEntityPersistListener( EntityPersistListener<T> l ) {
        listeners.remove( l );
    }

    /**
     * Fire the prePersist handler on any defined listeners
     *
     * @param entity the entity that is about to be saved
     */
    protected <R extends T> void firePrePersist( R entity ) {
        for ( EntityPersistListener<T> l : listeners ) {
            l.prePersist( entity );
        }
    }

    /**
     * Fire the prePersist handler on any defined listeners
     *
     * @param entities the list of entities about to be persisted
     */
    protected <R extends T> void firePrePersist( Collection<R> entities ) {
        for ( EntityPersistListener<T> l : listeners ) {
            l.prePersist( entities );
        }
    }

    /**
     * Fire the postPersist listener on any defined listeners
     *
     * @param entity the entity that was just persisted
     */
    protected <R extends T> void firePostPersist( R entity ) {
        for ( EntityPersistListener<T> l : listeners ) {
            l.postPersist( entity );
        }
    }

    /**
     * Fire the postPersist listener on any defined listeners
     *
     * @param entities the list of entities
     */
    protected <R extends T> void firePostPersist( Collection<R> entities ) {
        for ( EntityPersistListener<T> l : listeners ) {
            l.postPersist( entities );
        }
    }

    /**
     * @return the entity class type
     */
    @SuppressWarnings("unchecked")
    @Override
    public Class<T> getEntityClass() {
        return (Class<T>) ((ParameterizedType) getClass().getGenericSuperclass())
                              .getActualTypeArguments()[0];
    }

    /**
     * Save the entity to the repository.  This first calls prePersist on any entity
     * persistence listeners, then calls #doPersist(T), before calling postPersist listeners.
     *
     * @param entity the entity to save
     *
     * @return the persisted entity with any generated data updated in the entity.
     */
    @Override
    public <R extends T> R save( R entity ) {
        try {
            logger.trace( "ENTITY_SAVE: {}", entity );

            firePrePersist( entity );

            doPersist( entity );

            firePostPersist( entity );

            return entity;
        } catch ( RuntimeException re ) {
            logger.warn( "SAVE Failed", re );
            throw re;
        }
    }

    /**
     * Persist the specified events as a group.
     *
     * @param entities the list of entities to persist.
     *
     * @return the persisted events.
     *
     * @throws RuntimeException if the save for any entity fails
     */
    public <R extends T> List<R> save( Collection<R> entities ) {
        List<R> persisted = new ArrayList<>();
        try {
            logger.trace( "Saving {} entities",
                          entities != null ? entities.size() : 0 );
            if ( entities != null ) {
                entities.forEach( ( e ) -> logger.trace( "ENTITY_SAVE: {}",
                                                         e ) );

                firePrePersist( entities );

                persisted = doPersist( entities );

                firePostPersist( persisted );
            }
        } catch ( RuntimeException re ) {
            logger.debug( "SAVE Failed", re );
            throw re;
        }

        return persisted;
    }

    /**
     * @param entity the entity to delete
     */
    @Override
    public void delete( T entity ) {
        try {
            logger.trace( "Deleting entity {}", entity );

            doDelete( entity );
        } catch ( RuntimeException re ) {
            logger.debug( "DELETE Failed", re );
            throw re;
        }
    }

    /**
     * Persist the specified entity to the underlying data store
     *
     * @param entity the entity to persist
     *
     * @return the persisted entity with generated data (ids etc) updated.
     */
    abstract protected <R extends T> R doPersist( R entity );

    /**
     * Persist the specified entities to the underlying data store
     *
     * @param entities the entities to persist
     *
     * @return the list of persisted entities, each with any generated data (ids etc) updated.
     */
    abstract protected <R extends T> List<R> doPersist( Collection<R> entities );

    /**
     * Delete the specified entity
     *
     * @param entity the entity to delete
     */
    abstract protected void doDelete( T entity );

    // TODO: abstract protected int doDelete( QueryCriteria )
}
