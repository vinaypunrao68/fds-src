/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.query.builder;

import com.formationds.commons.model.abs.Context;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.om.repository.query.QueryCriteria;

import javax.persistence.EntityManager;
import javax.persistence.TypedQuery;
import javax.persistence.criteria.*;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

/**
 * @author ptinius
 */
@SuppressWarnings("UnusedDeclaration")
public class VolumeCriteriaQueryBuilder {
  private static final String TIMESTAMP = "timestamp";
  private static final String SERIES_TYPE = "key";
  private static final String CONTEXT = "volumeName";
  private static final String VALUE = "value";

  private List<Predicate> orPredicates;
  private List<Predicate> andPredicates;
  private Integer maxResults;
  private Integer firstResult;

  private final EntityManager em;
  private final CriteriaBuilder cb;
  private final CriteriaQuery<VolumeDatapoint> cq;
  private final Root<VolumeDatapoint> from;

  /**
   * @param entityManager the {@link javax.persistence.EntityManager}
   */
  public VolumeCriteriaQueryBuilder( final EntityManager entityManager ) {
    this.em = entityManager;
    this.cb = em.getCriteriaBuilder();
    this.cq = cb.createQuery( VolumeDatapoint.class );
    this.from = cq.from( VolumeDatapoint.class );
    this.andPredicates = new ArrayList<>();
    this.orPredicates = new ArrayList<>();
  }

  /**
   * @param start the {@link Date} representing the search starting period
   *
   * @return Returns {@link VolumeCriteriaQueryBuilder}
   */
  public VolumeCriteriaQueryBuilder withStart( final Date start ) {
    andPredicates.add( cb.lessThanOrEqualTo( from.<Long>get( TIMESTAMP ),
                                             start.getTime() ) );
    return this;
  }

  /**
   * @param end the {@link Date} representing the search end period
   *
   * @return Returns {@link VolumeCriteriaQueryBuilder}
   */
  public VolumeCriteriaQueryBuilder withEnd( final Date end ) {
    andPredicates.add( cb.greaterThanOrEqualTo( from.<Long>get( TIMESTAMP ),
                                                end.getTime() ) );
    return this;
  }

  /**
   * @param seriesType the {@link com.formationds.commons.model.type.Metrics}
   *
   * @return Returns {@link VolumeCriteriaQueryBuilder}
   */
  public VolumeCriteriaQueryBuilder withSeries( final Metrics seriesType ) {
    orPredicates.add( cb.equal( from.get( SERIES_TYPE ),
                                seriesType.key() ) );
    return this;
  }

  /**
   * @param contexts the {@link List} of {@link Context}
   *
   * @return Returns {@link VolumeCriteriaQueryBuilder}
   */
  public VolumeCriteriaQueryBuilder withContexts( final List<Context> contexts ) {
    final List<String> in = new ArrayList<>();
    for( final Context c : contexts ) {
      in.add( c.getContext() );
    }

    final Expression<String> expression = from.get( CONTEXT );
    andPredicates.add( expression.in( in.toArray( new String[ in.size() ] ) ) );
    return this;
  }

  /**
   * @param maxResults the {@code int} representing the maximum result set
   *                   size
   *
   * @return Returns {@link VolumeCriteriaQueryBuilder}
   */
  public VolumeCriteriaQueryBuilder maxResults( final int maxResults ) {
    this.maxResults = maxResults;
    return this;
  }

  /**
   * @param firstResult the {@code int} representing the starting results set
   *                    row id
   *
   * @return Returns {@link VolumeCriteriaQueryBuilder}
   */
  public VolumeCriteriaQueryBuilder firstResult( final int firstResult ) {
    this.firstResult = firstResult;
    return this;
  }

  /**
   * @param searchCriteria the {@link com.formationds.om.repository.query.QueryCriteria}
   *
   * @return Returns {@link VolumeCriteriaQueryBuilder}
   */
  public VolumeCriteriaQueryBuilder searchFor(
    final QueryCriteria searchCriteria ) {
    if( searchCriteria.getRange() != null &&
      searchCriteria.getRange()
                    .getStart() != null ) {
      this.withStart( searchCriteria.getRange()
                                    .getStart() );
    }

    if( searchCriteria.getRange() != null &&
      searchCriteria.getRange()
                    .getEnd() != null ) {
      this.withEnd( searchCriteria.getRange()
                                  .getEnd() );
    }

    if( searchCriteria.getSeriesType() != null &&
      !searchCriteria.getSeriesType()
                     .isEmpty() ) {
      searchCriteria.getSeriesType()
                    .forEach( this::withSeries );
    }

    if( searchCriteria.getContexts() != null &&
      !searchCriteria.getContexts()
                     .isEmpty() ) {
      this.withContexts( searchCriteria.getContexts() );
    }

    return this;
  }

  /**
   * @return Returns the {@link TypedQuery} representing the {@link
   * VolumeDatapoint}
   */
  public TypedQuery<VolumeDatapoint> build() {
    if( !andPredicates.isEmpty() && !orPredicates.isEmpty() ) {
      return em.createQuery(
        cq.select( from )
          .orderBy( cb.asc( from.get( CONTEXT ) ),
                    cb.asc( from.get( TIMESTAMP ) ) )
          .where(
            cb.and(
              andPredicates.toArray( new Predicate[ andPredicates.size() ] ) ),
            cb.or(
              orPredicates.toArray( new Predicate[ orPredicates.size() ] )
                 )
                )
                           );
    }

    if( andPredicates.isEmpty() && !orPredicates.isEmpty() ) {
      return em.createQuery(
        cq.select( from )
          .orderBy( cb.asc( from.get( CONTEXT ) ),
                    cb.asc( from.get( TIMESTAMP ) ) )
          .where(
            cb.or(
              orPredicates.toArray( new Predicate[ orPredicates.size() ] )
                 )
                )
                           );
    }

    if( !andPredicates.isEmpty() ) {
      return em.createQuery(
        cq.select( from )
          .orderBy( cb.asc( from.get( CONTEXT ) ),
                    cb.asc( from.get( TIMESTAMP ) ) )
          .where(
            cb.and(
              andPredicates.toArray( new Predicate[ andPredicates.size() ] ) )
                )
                           );
    }

    return em.createQuery(
      cq.select( from )
        .orderBy( cb.asc( from.get( CONTEXT ) ),
                  cb.asc( from.get( TIMESTAMP ) ) ) );
  }

  /**
   * @return Returns a {@link List} of {@link VolumeDatapoint} matching the
   * query criteria
   */
  public List<VolumeDatapoint> resultsList() {
    if( firstResult != null && maxResults != null ) {
      return build().setFirstResult( firstResult )
                    .setMaxResults( maxResults )
                    .getResultList();
    } else if( firstResult != null ) {
      return build().setFirstResult( firstResult )
                    .getResultList();
    } else if( maxResults != null ) {
      return build().setMaxResults( maxResults )
                    .getResultList();
    }

    return build().getResultList();
  }

  /**
   * @return Returns the matching {@link VolumeDatapoint}
   */
  public VolumeDatapoint singleResult() {
    if( firstResult != null && maxResults != null ) {
      return build().setFirstResult( firstResult )
                    .setMaxResults( maxResults )
                    .getSingleResult();
    } else if( firstResult != null ) {
      return build().setFirstResult( firstResult )
                    .getSingleResult();
    } else if( maxResults != null ) {
      return build().setMaxResults( maxResults )
                    .getSingleResult();
    }

    return build().getSingleResult();
  }
}
