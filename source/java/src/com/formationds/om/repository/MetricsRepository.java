/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository;

import com.formationds.commons.crud.JDORepository;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.helper.SingletonConfiguration;
import com.formationds.om.repository.query.QueryCriteria;
import com.formationds.om.repository.query.builder.MetricCriteriaQueryBuilder;
import com.formationds.om.repository.result.VolumeDatapointList;
import com.formationds.xdi.ConfigurationApi;
import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.jdo.Query;
import javax.persistence.EntityManager;
import javax.persistence.criteria.CriteriaBuilder;
import javax.persistence.criteria.CriteriaQuery;
import javax.persistence.criteria.Root;
import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
@SuppressWarnings( "UnusedDeclaration" )
public class MetricsRepository
  extends JDORepository<VolumeDatapoint, Long, VolumeDatapointList, QueryCriteria> {

  private static final String DBNAME = "var/db/metrics.odb";
  private static final String VOLUME_NAME = "volumeName";

  /**
   * Listener implementing prePersist to ensure that the volume id is set on each datapoint before
   * saving it.  When processing multiple datapoints, also aligns the timestamp to the first datapoint.
   */
  private static class MetricsEntityPersistListener implements EntityPersistListener<VolumeDatapoint> {
      private static final Logger logger = LoggerFactory.getLogger(MetricsRepository.class);
      public void prePersist(VolumeDatapoint dp) {
          try {
              long volid = SingletonConfigAPI.instance().api().getVolumeId(dp.getVolumeName());
              dp.setVolumeId(String.valueOf(volid));
          } catch (TException t) {
              throw new IllegalStateException("prePersist failed processing entity " + dp, t);
          }
      }

      public void prePersist(List<VolumeDatapoint> dps) {
          ConfigurationApi config = SingletonConfigAPI.instance().api();
          long timestamp = 0L;
          try {
              if (dps.isEmpty())
                  return;

              logger.trace("Setting volume id and timestamp for {} datapoints", dps.size());
              // set the volume id for each datapoint and align the timestamps
              for (VolumeDatapoint dp : dps) {
                  long volid = config.getVolumeId(dp.getVolumeName());
                  dp.setVolumeId(String.valueOf(volid));

// Note: commented out on master where this code was refactored from.
//                  if (timestamp <= 0L)
//                      timestamp = dp.getTimestamp();
//
//                  /*
//                   * syncing all the times to the first record. This will allow for us
//                   * to query for a time range and guarantee that all datapoints will
//                   * be aligned
//                   */
//                  dp.setTimestamp(timestamp);
              }
          } catch (TException t) {
              throw new IllegalStateException("prePersist failed processing volume data points.", t);
          }
      }
  }

  /**
   * default constructor
   */
  public MetricsRepository() {
    this( SingletonConfiguration.instance().getConfig().getFdsRoot() +
          File.separator +
          DBNAME );
  }

  /**
   * @param dbName the {@link String} representing the name and location of the
   *               repository
   */
  public MetricsRepository( final String dbName ) {
    super();
    initialize( dbName );

    super.addEntityPersistListener(new MetricsEntityPersistListener());
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
     * @param entity the search criteria
     *
     * @return the number of entities
     */
    @Override
    public long countAllBy( final VolumeDatapoint entity ) {
        return countAllBy( VOLUME_NAME, entity.getVolumeName() );
    }

  /**
   * @param criteria the search criteria
   *
   * @return Returns the search criteria results
   */
  @SuppressWarnings( "unchecked" )
  @Override
  public VolumeDatapointList query( final QueryCriteria criteria ) {
      EntityManager em = newEntityManager();
      try {
          final List<VolumeDatapoint> results = 
        		  new MetricCriteriaQueryBuilder(em).searchFor(criteria)
                                                    .build()
                                                    .getResultList();

          final VolumeDatapointList list = new VolumeDatapointList();
          results.stream().forEach(list::add);
          return list;
      } finally {
          em.close();
      }
  }

    /**
     * @return Returns the {@link Double} representing the calculated sum of logical bytes
     */
    public Double sumLogicalBytes() {
        EntityManager em = newEntityManager();
        try {
            final CriteriaBuilder cb = em.getCriteriaBuilder();
            final CriteriaQuery<String> cq = cb.createQuery( String.class );
            final Root<VolumeDatapoint> from = cq.from( VolumeDatapoint.class );

            cq.distinct( true ).select( from.get( "volumeId" ) );
            cq.where( cb.equal( from.get( "key" ), Metrics.LBYTES.key() ) );

            final List<VolumeDatapoint> datapoints = new ArrayList<>( );
            final List<String> volumeIds = em.createQuery( cq )
                                                   .getResultList();
            volumeIds.stream().forEach( ( vId ) -> datapoints.add(
              mostRecentOccurrenceBasedOnTimestamp( Long.valueOf( vId ),
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
    public Double sumPhysicalBytes() {
        EntityManager em = newEntityManager();
        try {
            final CriteriaBuilder cb = em.getCriteriaBuilder();
            final CriteriaQuery<String> cq = cb.createQuery( String.class );
            final Root<VolumeDatapoint> from = cq.from( VolumeDatapoint.class );

            cq.distinct(true )
              .select( from.get( "volumeId" ) );
            cq.where( cb.equal( from.get( "key" ), Metrics.PBYTES.key() ) );

            final List<VolumeDatapoint> datapoints = new ArrayList<>( );
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
     * @param volumeName  the {@link String} representing the volume name
     * @param metric the {@link Metrics}
     *
     * @return Returns the {@link VolumeDatapoint} representing the most recent
     */
    public VolumeDatapoint mostRecentOccurrenceBasedOnTimestamp(
      final String volumeName,
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
            final VolumeDatapoint[] previous = { null };
            datapoints.stream()
                      .forEach((dp) -> {
                          if (previous[0] == null) {
                              previous[0] = dp;
                          } else {
                              if (previous[0].getTimestamp() < dp.getTimestamp()) {
                                  previous[0] = dp;
                              }
                          }
                      });

            return previous[ 0 ];
        } finally {
            em.close();
        }
    }

    /**
     * @param volumeId the {@link Long} representing the volume id
     * @param metric the {@link Metrics}
     *
     * @return Returns the {@link VolumeDatapoint} representing the most recent
     */
    public VolumeDatapoint mostRecentOccurrenceBasedOnTimestamp(
      final Long volumeId,
      final Metrics metric ) {

        EntityManager em = newEntityManager();
        try {
            final CriteriaBuilder cb = em.getCriteriaBuilder();
            final CriteriaQuery<VolumeDatapoint> cq =
            cb.createQuery(VolumeDatapoint.class);
            final Root<VolumeDatapoint> from = cq.from(VolumeDatapoint.class);

            cq.select(from);
            cq.where(cb.equal(from.get("key"), metric.key()),
                     cb.and(cb.equal(from.get("volumeId"),
                                     volumeId.toString())));
            final List<VolumeDatapoint> datapoints = em.createQuery(cq)
                                                       .getResultList();
            final VolumeDatapoint[] previous = {null};
            datapoints.stream()
                      .forEach((dp) -> {
                          if (previous[0] == null) {
                              previous[0] = dp;
                          } else {
                              if (previous[0].getTimestamp() < dp.getTimestamp()) {
                                  previous[0] = dp;
                              }
                          }
                      });

            return previous[0];
        } finally {
            em.close();
        }
    }

    /**
     * @param volumeId the {@link Long} representing the volume id
     * @param metric the {@link Metrics}
     *
     * @return Returns the {@link VolumeDatapoint} representing the most recent
     */
    public VolumeDatapoint leastRecentOccurrenceBasedOnTimestamp(
        final Long volumeId,
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
            final VolumeDatapoint[] previous = { null };
            datapoints.stream()
                      .forEach((dp) -> {
                          if (previous[0] == null) {
                              previous[0] = dp;
                          } else {
                              if (previous[0].getTimestamp() > dp.getTimestamp()) {
                                  previous[0] = dp;
                              }
                          }
                      });

            return previous[ 0 ];
        } finally {
            em.close();
        }
    }

    /**
     * @param volumeName  the {@link String} representing the volume name
     * @param metric the {@link Metrics}
     *
     * @return Returns the {@link VolumeDatapoint} representing the most recent
     */
    public VolumeDatapoint leastRecentOccurrenceBasedOnTimestamp(
        final String volumeName,
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
            final VolumeDatapoint[] previous = { null };
            datapoints.stream()
                      .forEach((dp) -> {
                          if (previous[0] == null) {
                              previous[0] = dp;
                          } else {
                              if (previous[0].getTimestamp() > dp.getTimestamp()) {
                                  previous[0] = dp;
                              }
                          }
                      });

            return previous[ 0 ];
        } finally {
            em.close();
        }
    }
}
