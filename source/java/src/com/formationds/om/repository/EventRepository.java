/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository;

import com.formationds.commons.crud.JDORepository;
import com.formationds.commons.model.Events;
import com.formationds.commons.model.entity.Event;
import com.formationds.commons.model.entity.EventQuery;
import com.formationds.commons.model.entity.VolumeDatapoint;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.jdo.JDOHelper;
import javax.jdo.Query;
import javax.persistence.EntityManagerFactory;
import javax.persistence.Persistence;
import java.util.Collection;
import java.util.List;
import java.util.Properties;

// TODO: not implemented
public class EventRepository extends JDORepository<Event, Long, Events, EventQuery> {

    private static final transient Logger logger = LoggerFactory.getLogger(EventRepository.class);

    // TODO: default should be based on fdsroot (is this available somewhere?)
    private static final String DBNAME = "/fds/var/db/events.odb";

    /**
     * default constructor
     */
    public EventRepository() {
        this( DBNAME );
    }

    /**
     * @param dbName the {@link String} representing the name and location of the
     *               repository
     */
    public EventRepository( final String dbName ) {
        super();
        initialize( dbName );
    }

    @Override
    protected void initialize(String dbName) {

        final Properties properties = new Properties();
        properties.setProperty( "javax.jdo.PersistenceManagerFactoryClass", "com.objectdb.jdo.PMF" );
        properties.setProperty( "javax.jdo.option.ConnectionURL", dbName );

        factory( JDOHelper.getPersistenceManagerFactory(properties) );
        manager( factory().getPersistenceManager() );

        final EntityManagerFactory emf = Persistence.createEntityManagerFactory(dbName);
        entity( emf.createEntityManager() );
    }

    @Override
    public Event findById(Long id) {
        final Query query = manager().newQuery( Event.class, "id == :id" );
        query.setUnique( true );
        try {
            return ( Event ) query.execute( id );
        } finally {
            query.closeAll();
        }
    }

    @Override
    public List<Event> findAll() {
        return (List<Event>) manager().newQuery(Event.class).execute();
    }

    @Override
    public long countAll() {
        long count = 0;
        // TODO: is there really no way to do a "select count(*)"?
        final Query query = manager().newQuery( Event.class );
        try {
            query.compile();
            count = ( (Collection) query.execute() ).size();
        } finally {
            query.closeAll();
        }

        return count;
    }

    @Override
    public long countAllBy(Event entity) {
        return 0;
    }

    @Override
    public Event save(Event entity) {
        try {
            // ugh! I know slf4j came first, but I wish it used String.format syntax.
            logger.trace("Saving event {}", entity);
            manager().currentTransaction().begin();

            Event persisted = manager().makePersistent(entity);

            manager().currentTransaction().commit();
            logger.trace("Event saved {}", entity);
            return persisted;
        } catch (RuntimeException re) {
            logger.warn("SAVE Failed.  Rolling back transaction.");
            logger.debug("SAVE Failed", re);
            manager().currentTransaction().rollback();
            throw re;
        }
// TODO: ok. I can see the logic of avoiding an ugly catch/rethrow, but it seems
// far more conventional to do that then checking a status in a finally block.
//        finally {
//            if( manager().currentTransaction().isActive() ) {
//                if (logger.isTraceEnabled())
//                    logger.trace( "SAVE ROLLING BACK: " + entity.toString() );
//
//
//                if (logger.isTraceEnabled())
//                    logger.trace( "SAVE ROLLED BACK: " + entity.toString() );
//            }
//        }
    }

    @Override
    public Events query(EventQuery queryCriteria) {
        return null;
    }

    @Override
    public void delete(Event entity) {
        try {
            // ugh! I know slf4j came first, but I wish it used String.format syntax.
            logger.trace("Saving event {}", entity);
            manager().currentTransaction().begin();

            manager().deletePersistent(entity);

            manager().currentTransaction().commit();
            logger.trace("Event saved {}", entity);
        } catch (RuntimeException re) {
            logger.warn("DELETE Failed.  Rolling back transaction.");
            logger.debug("DELETE Failed", re);
            manager().currentTransaction().rollback();
            throw re;
        }
// TODO: ok. I can see the logic of avoiding an ugly catch/rethrow, but it seems
// far more conventional to do that then checking a status in a finally block.
//        finally {
//            if( manager().currentTransaction().isActive() ) {
//                if (logger.isTraceEnabled())
//                    logger.trace( "DELETE ROLLING BACK: " + entity.toString() );
//
//
//                if (logger.isTraceEnabled())
//                    logger.trace( "DELETE ROLLED BACK: " + entity.toString() );
//            }
//        }

    }
}
