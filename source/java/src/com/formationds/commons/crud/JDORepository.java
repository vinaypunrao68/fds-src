/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.crud;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.jdo.JDOHelper;
import javax.jdo.PersistenceManager;
import javax.jdo.PersistenceManagerFactory;
import javax.jdo.Query;
import javax.persistence.EntityManager;
import javax.persistence.EntityManagerFactory;
import javax.persistence.Persistence;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Properties;

/**
 * @author ptinius
 */
public abstract class JDORepository<T, PK extends Serializable>
    extends AbstractRepository<T, PK>
    implements CRUDRepository<T, PK> {

    private String                    dbName;
    private PersistenceManagerFactory factory;
    private PersistenceManager        manager;

    private EntityManagerFactory entityManagerFactory;

    // use non-static logger to tie it to the concrete subclass.
    protected final Logger logger;

    protected JDORepository() {
        logger = LoggerFactory.getLogger( this.getClass() );
    }

    /**
     * @return the number of entities
     */
    @Override
    public long countAll() {
        long count = 0;
        final Query query = manager().newQuery( getEntityClass() );
        try {
            query.compile();
            count = ((Collection) query.execute()).size();
        } finally {
            query.closeAll();
        }

        return count;
    }

    /**
     * @param paramName  the {@link String} representing the parameter name
     * @param paramValue the {@link String} representing the parameter value
     *
     * @return the number of entities
     */
    public long countAllBy( final String paramName, final String paramValue ) {
        int count = 0;
        Query query = null;
        try {
            query = manager().newQuery( getEntityClass() );
            query.setFilter( paramName + " == '" + paramValue + "'" );

            count = ((Collection) query.execute()).size();
        } finally {
            if ( query != null ) {
                query.closeAll();
            }
        }

        return count;
    }

    /**
     * @param entity the entity to save
     *
     * @return Returns the saved entity
     */
    @Override
    synchronized protected <R extends T>  R doPersist( final R entity ) {
        try {
            if ( !manager().currentTransaction().isActive() ) {
                manager().currentTransaction().begin();
            }
            manager().makePersistent( entity );

            if ( manager().currentTransaction().isActive() ) {
                manager().currentTransaction().commit();
            }

            return entity;
        } catch ( RuntimeException re ) {
            if ( manager().currentTransaction().isActive() ) {
                logger.warn( "Rolling back failed transaction." );
                manager().currentTransaction().rollback();
            }
            throw re;
        }
    }

    /**
     * Persist the specified events in a transaction.
     *
     * @param entities
     *
     * @return the persisted events.
     *
     * @throws RuntimeException if the save for any entity fails
     */
    @Override
    synchronized protected <R extends T> List<R> doPersist( Collection<R> entities ) {
        List<R> persisted = new ArrayList<>();
        try {
            if ( entities != null ) {

                if ( !manager().currentTransaction().isActive() ) {
                    manager().currentTransaction().begin();
                }
                persisted.addAll( manager().makePersistentAll( entities ) );

                if ( manager().currentTransaction().isActive() ) {
                    manager().currentTransaction().commit();
                }
            }
        } catch ( RuntimeException re ) {
            if ( manager().currentTransaction().isActive() ) {
                logger.warn( "SAVE Failed.  Rolling back transaction." );
                manager().currentTransaction().rollback();
            }
            throw re;
        }

        return persisted;
    }

    @Override
    synchronized protected void doDelete( T entity ) {
        try {
            manager().currentTransaction()
                     .begin();
            manager().deletePersistent( entity );
            manager().currentTransaction()
                     .commit();
        } catch ( RuntimeException re ) {
            if ( manager().currentTransaction()
                          .isActive() ) {
                logger.warn( "DELETE Failed.  Rolling back transaction." );
                manager().currentTransaction()
                         .rollback();
            }

            throw re;
        }
    }

    /**
     * close the repository
     */
    @Override
    public void close() {
        // ignoring errors on close.  Although this is likely to indicate either
        // a shutdown condition or a programming error such as calling close twice,
        // there is no real impact or recovery actions to take
        try { entityManagerFactory.close(); } catch ( Throwable t ) { /* ignore on close */ }
        try { manager.close(); } catch ( Throwable t ) { /* ignore on close */ }
        try { factory.close(); } catch ( Throwable t ) { /* ignore on close */ }
    }

    /**
     * @param dbName the {@link String} representing the name and location of the
     *               repository
     */
    protected void initialize( final String dbName ) {
        this.dbName = dbName;
        final Properties properties = new Properties();
        properties.setProperty( "javax.jdo.PersistenceManagerFactoryClass", "com.objectdb.jdo.PMF" );
        properties.setProperty( "javax.jdo.option.ConnectionURL", dbName );

        open( properties );
    }

    /**
     * Open the repository with the specified properties
     *
     * @param properties the set of properties to configure the repository manager
     *
     * @throws IllegalStateException if already open.  Make sure to close first.
     */
    public void open( Properties properties ) {
        if ( manager != null ) {
            throw new IllegalStateException( "Repository is already open." );
        }
        factory( JDOHelper.getPersistenceManagerFactory( properties ) );
        manager( factory().getPersistenceManager() );

        final EntityManagerFactory emf = Persistence.createEntityManagerFactory( dbName );

        entityManagerFactory( emf );

        initShutdownHook();
    }

    /**
     * Initialize the shutdown hook to close the repository.
     */
    private void initShutdownHook() {
        // ensure this repository is closed at shutdown
        Runtime.getRuntime().addShutdownHook( new Thread() {
            @Override
            public void run() { close(); }
        } );
    }

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
     * @return Returns the {@link EntityManager}
     */
    public EntityManager newEntityManager() {
        return entityManagerFactory.createEntityManager();
    }

    /**
     * @param entity the {@link javax.persistence.EntityManager}
     */
    protected void entityManagerFactory( final EntityManagerFactory entity ) {
        this.entityManagerFactory = entity;
    }
}
