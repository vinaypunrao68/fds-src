/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository;

import com.formationds.apis.VolumeStatus;
import com.formationds.commons.crud.JDORepository;
import com.formationds.commons.model.entity.IVolumeDatapoint;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.om.helper.SingletonConfiguration;
import com.formationds.om.repository.query.QueryCriteria;
import com.formationds.om.repository.query.builder.MetricCriteriaQueryBuilder;
import com.formationds.om.repository.result.VolumeDatapointList;

import javax.jdo.Query;
import javax.persistence.EntityManager;
import javax.persistence.criteria.CriteriaBuilder;
import javax.persistence.criteria.CriteriaQuery;
import javax.persistence.criteria.Root;
import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

/**
 * @author ptinius
 */
@SuppressWarnings("UnusedDeclaration")
public class JDOMetricsRepository extends JDORepository<IVolumeDatapoint, Long> implements MetricRepository {

    private static final String DBNAME      = "var/db/metrics.odb";
    private static final String VOLUME_NAME = "volumeName";

    /**
     * default constructor
     */
    public JDOMetricsRepository() {
        this( SingletonConfiguration.instance().getConfig().getFdsRoot() +
              File.separator +
              DBNAME );
    }

    /**
     * @param dbName the {@link String} representing the name and location of the
     *               repository
     */
    public JDOMetricsRepository( final String dbName ) {
        super();
        initialize( dbName );
        super.addEntityPersistListener( new MetricsEntityPersistListener() );
    }

    /**
     * @param id the primary key
     *
     * @return the entity
     */
    public VolumeDatapoint findById( final Long id ) {
        final Query query = manager().newQuery( VolumeDatapoint.class,
                                                "id == :id" );
        query.setUnique( true );
        try {
            return (VolumeDatapoint) query.execute( id );
        } finally {
            query.closeAll();
        }
    }

    /**
     * @param entity the search criteria
     *
     * @return the number of entities
     */
    @Override
    public long countAllBy( final IVolumeDatapoint entity ) {
        return countAllBy( VOLUME_NAME, entity.getVolumeName() );
    }

    /**
     * @param criteria the search criteria
     *
     * @return Returns the search criteria results
     */
    @SuppressWarnings("unchecked")
    @Override
    public VolumeDatapointList query( final QueryCriteria criteria ) {
        EntityManager em = newEntityManager();
        try {
            final List<VolumeDatapoint> results =
                new MetricCriteriaQueryBuilder( em ).searchFor( criteria )
                                                    .build()
                                                    .getResultList();

            final VolumeDatapointList list = new VolumeDatapointList();
            results.stream().forEach( list::add );
            return list;
        } finally {
            em.close();
        }
    }

    /**
     * @return Returns the {@link Double} representing the calculated sum of logical bytes
     */
    @Override
    public Double sumLogicalBytes() {

        EntityManager em = newEntityManager();
        try {
            final CriteriaBuilder cb = em.getCriteriaBuilder();
            final CriteriaQuery<String> cq = cb.createQuery( String.class );
            final Root<VolumeDatapoint> from = cq.from( VolumeDatapoint.class );

            cq.distinct( true ).select( from.get( "volumeId" ) );
            cq.where( cb.equal( from.get( "key" ), Metrics.LBYTES.key() ) );

            final List<VolumeDatapoint> datapoints = new ArrayList<>();
            final List<String> volumeIds = em.createQuery( cq )
                                             .getResultList();
            volumeIds.stream().forEach( ( vId ) ->
                                            datapoints.add( mostRecentOccurrenceBasedOnTimestamp( Long.valueOf( vId ),
                                                                                                  Metrics.LBYTES ) ) );

            return datapoints.stream()
                             .mapToDouble( VolumeDatapoint::getValue )
                             .summaryStatistics()
                             .getSum();
        } finally {
            em.close();
        }
    }

    /**
     * @return Returns the {@link Double} representing the calculated sum of physical bytes
     */
    @Override
    public Double sumPhysicalBytes() {
        EntityManager em = newEntityManager();
        try {
            final CriteriaBuilder cb = em.getCriteriaBuilder();
            final CriteriaQuery<String> cq = cb.createQuery( String.class );
            final Root<VolumeDatapoint> from = cq.from( VolumeDatapoint.class );

            cq.distinct( true )
              .select( from.get( "volumeId" ) );
            cq.where( cb.equal( from.get( "key" ), Metrics.PBYTES.key() ) );

            final List<VolumeDatapoint> datapoints = new ArrayList<>();
            final List<String> volumeIds = em.createQuery( cq )
                                             .getResultList();
            volumeIds.stream().forEach( ( vId ) -> datapoints.add(
                                                                     mostRecentOccurrenceBasedOnTimestamp( Long.valueOf( vId ),
                                                                                                           Metrics.PBYTES ) ) );

            return datapoints.stream()
                             .mapToDouble( VolumeDatapoint::getValue )
                             .summaryStatistics()
                             .getSum();
        } finally {
            em.close();
        }
    }

    /**
     * @param volumeName the {@link String} representing the volume name
     * @param metric     the {@link Metrics}
     *
     * @return Returns the {@link VolumeDatapoint} representing the most recent
     */
    @Override
    public VolumeDatapoint mostRecentOccurrenceBasedOnTimestamp( final String volumeName,
                                                                 final Metrics metric ) {

        EntityManager em = newEntityManager();
        try {
            final CriteriaBuilder cb = em.getCriteriaBuilder();
            final CriteriaQuery<VolumeDatapoint> cq =
                cb.createQuery( VolumeDatapoint.class );
            final Root<VolumeDatapoint> from = cq.from( VolumeDatapoint.class );

            cq.select( from );
            cq.where( cb.equal( from.get( "key" ), metric.key() ),
                      cb.and( cb.equal( from.get( "volumeName" ), volumeName ) ) );
            final List<VolumeDatapoint> datapoints = em.createQuery( cq )
                                                       .getResultList();
            final VolumeDatapoint[] previous = {null};
            datapoints.stream()
                      .forEach( ( dp ) -> {
                          if ( previous[0] == null ) {
                              previous[0] = dp;
                          } else {
                              if ( previous[0].getTimestamp() < dp.getTimestamp() ) {
                                  previous[0] = dp;
                              }
                          }
                      } );

            return previous[0];
        } finally {
            em.close();
        }
    }

    /**
     * @param volumeId the {@link Long} representing the volume id
     * @param metric   the {@link Metrics}
     *
     * @return Returns the {@link VolumeDatapoint} representing the most recent
     */
    @Override
    public VolumeDatapoint mostRecentOccurrenceBasedOnTimestamp( final Long volumeId,
                                                                 final Metrics metric ) {

        EntityManager em = newEntityManager();
        try {
            final CriteriaBuilder cb = em.getCriteriaBuilder();
            final CriteriaQuery<VolumeDatapoint> cq =
                cb.createQuery( VolumeDatapoint.class );
            final Root<VolumeDatapoint> from = cq.from( VolumeDatapoint.class );

            cq.select( from );
            cq.where( cb.equal( from.get( "key" ), metric.key() ),
                      cb.and( cb.equal( from.get( "volumeId" ),
                                        volumeId.toString() ) ) );
            final List<VolumeDatapoint> datapoints = em.createQuery( cq )
                                                       .getResultList();
            final VolumeDatapoint[] previous = {null};
            datapoints.stream()
                      .forEach( ( dp ) -> {
                          if ( previous[0] == null ) {
                              previous[0] = dp;
                          } else {
                              if ( previous[0].getTimestamp() < dp.getTimestamp() ) {
                                  previous[0] = dp;
                              }
                          }
                      } );

            return previous[0];
        } finally {
            em.close();
        }
    }

    /**
     * @param volumeId the {@link Long} representing the volume id
     * @param metric   the {@link Metrics}
     *
     * @return Returns the {@link VolumeDatapoint} representing the least recent
     */
    @Override
    public VolumeDatapoint leastRecentOccurrenceBasedOnTimestamp( final Long volumeId,
                                                                  final Metrics metric ) {

        EntityManager em = newEntityManager();
        try {
            final CriteriaBuilder cb = em.getCriteriaBuilder();
            final CriteriaQuery<VolumeDatapoint> cq =
                cb.createQuery( VolumeDatapoint.class );
            final Root<VolumeDatapoint> from = cq.from( VolumeDatapoint.class );

            cq.select( from );
            cq.where( cb.equal( from.get( "key" ), metric.key() ),
                      cb.and( cb.equal( from.get( "volumeId" ),
                                        volumeId.toString() ) ) );
            final List<VolumeDatapoint> datapoints = em.createQuery( cq )
                                                       .getResultList();
            final VolumeDatapoint[] previous = {null};
            datapoints.stream()
                      .forEach( ( dp ) -> {
                          if ( previous[0] == null ) {
                              previous[0] = dp;
                          } else {
                              if ( previous[0].getTimestamp() > dp.getTimestamp() ) {
                                  previous[0] = dp;
                              }
                          }
                      } );

            return previous[0];
        } finally {
            em.close();
        }
    }

    /**
     * @param volumeName the {@link String} representing the volume name
     * @param metric     the {@link Metrics}
     *
     * @return Returns the {@link VolumeDatapoint} representing the least recent
     */
    @Override
    public VolumeDatapoint leastRecentOccurrenceBasedOnTimestamp( final String volumeName,
                                                                  final Metrics metric ) {

        EntityManager em = newEntityManager();
        try {
            final CriteriaBuilder cb = em.getCriteriaBuilder();
            final CriteriaQuery<VolumeDatapoint> cq =
                cb.createQuery( VolumeDatapoint.class );
            final Root<VolumeDatapoint> from = cq.from( VolumeDatapoint.class );

            cq.select( from );
            cq.where( cb.equal( from.get( "key" ), metric.key() ),
                      cb.and( cb.equal( from.get( "volumeName" ), volumeName ) ) );
            final List<VolumeDatapoint> datapoints = em.createQuery( cq )
                                                       .getResultList();
            final VolumeDatapoint[] previous = {null};
            datapoints.stream()
                      .forEach( ( dp ) -> {
                          if ( previous[0] == null ) {
                              previous[0] = dp;
                          } else {
                              if ( previous[0].getTimestamp() > dp.getTimestamp() ) {
                                  previous[0] = dp;
                              }
                          }
                      } );

            return previous[0];
        } finally {
            em.close();
        }
    }

    @Override
    public Optional<VolumeStatus> getLatestVolumeStatus( final Long volumeId ) {

        final VolumeDatapoint blobs =
            mostRecentOccurrenceBasedOnTimestamp( volumeId, Metrics.BLOBS );
        final VolumeDatapoint usage =
            mostRecentOccurrenceBasedOnTimestamp( volumeId, Metrics.PBYTES );

        return volumeStatus( blobs, usage );
    }

    @Override
    public Optional<VolumeStatus> getLatestVolumeStatus( final String volumeName ) {

        final VolumeDatapoint blobs =
            mostRecentOccurrenceBasedOnTimestamp( volumeName, Metrics.BLOBS );
        final VolumeDatapoint usage =
            mostRecentOccurrenceBasedOnTimestamp( volumeName, Metrics.PBYTES );

        return volumeStatus( blobs, usage );
    }

    protected Optional<VolumeStatus> volumeStatus( final VolumeDatapoint blobs, final VolumeDatapoint usage ) {
        if ( (blobs != null) && (usage != null) ) {

            return Optional.of( new VolumeStatus( blobs.getValue().longValue(),
                                                  usage.getValue().longValue() ) );
        } else if ( (blobs == null) && (usage != null) ) {

            return Optional.of( new VolumeStatus( 0L,
                                                  usage.getValue().longValue() ) );
        } else if ( blobs != null ) {

            return Optional.of( new VolumeStatus( blobs.getValue().longValue(),
                                                  0L ) );
        }

        return Optional.empty();
    }
}