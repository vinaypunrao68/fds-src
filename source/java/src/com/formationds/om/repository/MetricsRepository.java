/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository;

import com.formationds.commons.crud.JDORepository;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.om.repository.query.QueryCriteria;
import com.formationds.om.repository.query.builder.VolumeCriteriaQueryBuilder;
import com.formationds.om.repository.result.VolumeDatapointList;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.jdo.JDOHelper;
import javax.jdo.Query;
import javax.persistence.EntityManagerFactory;
import javax.persistence.Persistence;
import java.util.Collection;
import java.util.List;
import java.util.Properties;

/**
 * @author ptinius
 */
public class MetricsRepository
  extends JDORepository<VolumeDatapoint, Long, VolumeDatapointList, QueryCriteria> {

  private static final transient Logger logger =
    LoggerFactory.getLogger( MetricsRepository.class );

  private static final String DBNAME = "/fds/var/db/metrics.odb";
  private static final String VOLUME_NAME = "volumeName";

  /**
   * default constructor
   */
  public MetricsRepository() {
    this( DBNAME );
  }

  /**
   * @param dbName the {@link String} representing the name and location of the
   *               repository
   */
  public MetricsRepository( final String dbName ) {
    super();

    initialize( dbName );

    Runtime.getRuntime()
           .addShutdownHook( new Thread() {
             /**
              * make sure we close the repository
              */
             @Override
             public void run() {
               close();
             }
           } );
  }

  /**
   * @param id the primary key
   *
   * @return the entity
   */
  @Override
  public VolumeDatapoint findById( final Long id ) {
    final Query query = manager().newQuery( VolumeDatapoint.class,
                                            "id == :id" );
    query.setUnique( true );
    try {
      return ( VolumeDatapoint ) query.execute( id );
    } finally {
      query.closeAll();
    }
  }

  /**
   * @return the list of entities
   */
  @SuppressWarnings("unchecked")
  @Override
  public List<VolumeDatapoint> findAll() {
    return
      ( List<VolumeDatapoint> ) manager().newQuery( VolumeDatapoint.class )
                                         .execute();
  }

  /**
   * @return the number of entities
   */
  @Override
  public long countAll() {
    long count = 0;
    final Query query = manager().newQuery( VolumeDatapoint.class );
    try {
      query.compile();
      count = ( ( Collection ) query.execute() ).size();
    } finally {
      query.closeAll();
    }

    return count;
  }

  /**
   * @param entity the search criteria
   *
   * @return the number of entities
   */
  @Override
  public long countAllBy( final VolumeDatapoint entity ) {
    return countAllBy( VOLUME_NAME, entity.getVolumeName() );
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
      query = manager().newQuery( VolumeDatapoint.class );
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
   * @param entity the entity to save
   *
   * @return Returns the saved entity
   */
  @Override
  public VolumeDatapoint save( final VolumeDatapoint entity ) {
    try {
      manager().currentTransaction()
               .begin();
      manager().makePersistent( entity );
      manager().currentTransaction()
               .commit();
      return entity;
    } finally {
      if( manager().currentTransaction()
                   .isActive() ) {
        logger.trace( "SAVE ROLLING BACK: " + entity.toString() );
        manager().currentTransaction()
                 .rollback();
        logger.trace( "SAVE ROLLED BACK: " + entity.toString() );
      }
    }
  }

  /**
   * @param criteria the search criteria
   *
   * @return Returns the search criteria results
   */
  @Override
  public VolumeDatapointList query( final QueryCriteria criteria ) {
    final List<VolumeDatapoint> results =
      ( new VolumeCriteriaQueryBuilder( entity() ).searchFor( criteria )
                                                  .build() )
        .getResultList();

    final VolumeDatapointList list = new VolumeDatapointList();
    results.stream()
           .forEach( new VolumeDatapointList()::add );
    return list;
  }

  /**
   * @param entity the entity to delete
   */
  @Override
  public void delete( final VolumeDatapoint entity ) {
    try {
      manager().currentTransaction()
               .begin();
      manager().deletePersistent( entity );
      manager().currentTransaction()
               .commit();
    } finally {
      if( manager().currentTransaction()
                   .isActive() ) {
        logger.trace( "DELETE ROLLING BACK: " + entity.toString() );
        manager().currentTransaction()
                 .rollback();
        logger.trace( "DELETE ROLLED BACK: " + entity.toString() );
      }
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
  @Override
  protected void initialize( final String dbName ) {
    final Properties properties = new Properties();
    properties.setProperty( "javax.jdo.PersistenceManagerFactoryClass",
                            "com.objectdb.jdo.PMF" );
    properties.setProperty( "javax.jdo.option.ConnectionURL",
                            dbName );

    factory( JDOHelper.getPersistenceManagerFactory( properties ) );
    manager( factory().getPersistenceManager() );

    final EntityManagerFactory emf =
      Persistence.createEntityManagerFactory( dbName );

    entity( emf.createEntityManager() );
  }
}
