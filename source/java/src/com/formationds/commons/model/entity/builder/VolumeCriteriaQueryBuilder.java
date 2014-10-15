/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.entity.builder;

import com.formationds.commons.model.Context;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.commons.model.type.ResultType;

import javax.persistence.EntityManager;
import javax.persistence.TypedQuery;
import javax.persistence.criteria.*;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

/**
 * @author ptinius
 */
public class VolumeCriteriaQueryBuilder {
  private static final String UNSUPPORTED_RESULT_TYPE =
    "Unsupported Result Type '%s'.";

  private static final String TIMESTAMP = "timestamp";
  private static final String SERIES_TYPE = "key";
  private static final String CONTEXT = "volumeName";
  private static final String VALUE = "value";

  private List<Predicate> predicates;
  private List<Selection<?>> selections;
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
    this.predicates = new ArrayList<>();
    this.selections = new ArrayList<>();
    this.selections.add( from );
  }

  public VolumeCriteriaQueryBuilder withStart( final Date start ) {
    predicates.add( cb.greaterThanOrEqualTo( from.get( TIMESTAMP ),
                                             start.getTime() ) );
    return this;
  }

  public VolumeCriteriaQueryBuilder withEnd( final Date end ) {
    predicates.add( cb.lessThanOrEqualTo( from.get( TIMESTAMP ),
                                          end.getTime() ) );
    return this;
  }

  public VolumeCriteriaQueryBuilder withSeries( final Metrics seriesType ) {
    predicates.add( cb.equal( from.get( SERIES_TYPE ),
                              seriesType.key() ) );
    return this;
  }

  public VolumeCriteriaQueryBuilder withContexts( final List<Context> contexts ) {
    final List<String> in = new ArrayList<>();
    for( final Context c : contexts ) {
      in.add( c.getContext() );
    }

    final Expression<String> expression = from.get( CONTEXT );
    predicates.add( expression.in( in.toArray( new String[ in.size() ] ) ) );
    return this;
  }

  public VolumeCriteriaQueryBuilder maxResults( final int maxResults ) {
    this.maxResults = maxResults;
    return this;
  }

  public VolumeCriteriaQueryBuilder fistResult( final int firstResult ) {
    this.firstResult = firstResult;
    return this;
  }

  public VolumeCriteriaQueryBuilder withResultType( final ResultType type ) {
    switch( type.name() ) {
      case "MIN":
        selections.add( cb.min( from.get( VALUE ) ) );
        break;
      case "AVG":
        selections.add( cb.avg( from.get( VALUE ) ) );
        break;
      case "MAX":
        selections.add( cb.max( from.get( VALUE ) ) );
        break;
      case "SUM":
        selections.add( cb.sum( from.get( VALUE ) ) );
        break;
      default:
        throw new IllegalArgumentException( String.format( UNSUPPORTED_RESULT_TYPE, type ) );
    }

    return this;
  }

  /**
   * @return Returns the {@link Object}
   */
  public TypedQuery<VolumeDatapoint> build() {

    return em.createQuery(
      cq.multiselect( selections )
        .where(
          cb.and(
            predicates.toArray( new Predicate[ predicates.size() ] ) ) ) );
  }

  public List<VolumeDatapoint> execute() {
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
}
