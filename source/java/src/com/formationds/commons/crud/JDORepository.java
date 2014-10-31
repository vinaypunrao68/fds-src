/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.crud;

import com.formationds.commons.model.entity.Event;
import com.formationds.commons.model.entity.VolumeDatapoint;
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
import java.lang.reflect.ParameterizedType;
import java.util.*;

/**
 * @author ptinius
 */
public abstract class JDORepository<T,
                                    PrimaryKey extends Serializable,
                                    QueryResults extends SearchResults,
                                    QueryCriteria extends SearchCriteria>
        implements CRUDRepository<T, PrimaryKey, QueryResults, QueryCriteria> {

  private PersistenceManagerFactory factory;
  private PersistenceManager manager;
  private EntityManager entity;

  // use non-static logger to tie it to the concrete subclass.
  private final Logger logger;

  protected JDORepository() {
      logger = LoggerFactory.getLogger(this.getClass());
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
            count = ( (Collection) query.execute() ).size();
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

            count = ( ( Collection ) query.execute() ).size();
        } finally {
            if( query != null ) {
                query.closeAll();
            }
        }

        return count;
    }

    /**
     * @return the list of entities
     */
    @SuppressWarnings("unchecked")
    @Override
    public List<T> findAll() {
        return (List<T>) manager().newQuery(getEntityClass()).execute();
    }

    /**
     * @param entity the entity to save
     *
     * @return Returns the saved entity
     */
    @Override
    public T save( final T entity ) {
        try {
            logger.trace("Saving entity {}", entity);
            manager().currentTransaction().begin();
            manager().makePersistent( entity );
            manager().currentTransaction().commit();
            logger.trace("Saved entity {}", entity);
            return entity;
        } catch (RuntimeException re) {
            logger.warn("SAVE Failed.  Rolling back transaction.");
            logger.debug("SAVE Failed", re);
            manager().currentTransaction().rollback();
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
    public List<T> save(Collection<T> entities) {
        int cnt = (entities != null ? entities.size() : 0);
        List<T> persisted = new ArrayList<>(cnt);
        try {
            logger.trace("Saving entities {}", entities);
            manager().currentTransaction().begin();

            for (T entity : entities) {
                T pe = manager().makePersistent(entity);
                persisted.add(pe);
            }

            manager().currentTransaction().commit();
            logger.trace("Saved entities {}", entities);
            return persisted;
        } catch (RuntimeException re) {
            logger.warn("SAVE Failed.  Rolling back transaction.");
            logger.debug("SAVE Failed", re);
            manager().currentTransaction().rollback();
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
    public List<T> save(T... entities) {
        return (entities != null ? save(Arrays.asList(entities)) : new ArrayList<>(0));
    }

    @Override
    public void delete(T entity) {
        try {
            logger.trace("Deleting entity {}", entity);
            manager().currentTransaction().begin();
            manager().deletePersistent(entity);
            manager().currentTransaction().commit();
            logger.trace("Deleted entity {}", entity);
        } catch (RuntimeException re) {
            logger.warn("DELETE Failed.  Rolling back transaction.");
            logger.debug("DELETE Failed", re);
            manager().currentTransaction().rollback();
            throw re;
        }
    }

    /**
     * close the repository
     */
    @Override
    public void close() {
        entity().close();
        manager().close();
        factory().close();
    }

    /**
     * @param dbName the {@link String} representing the name and location of the
     *               repository
     */
    protected void initialize( final String dbName ) {
        final Properties properties = new Properties();
        properties.setProperty( "javax.jdo.PersistenceManagerFactoryClass", "com.objectdb.jdo.PMF" );
        properties.setProperty( "javax.jdo.option.ConnectionURL", dbName );

        factory( JDOHelper.getPersistenceManagerFactory(properties) );
        manager( factory().getPersistenceManager() );

        final EntityManagerFactory emf = Persistence.createEntityManagerFactory(dbName);

        entity( emf.createEntityManager() );

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
}
