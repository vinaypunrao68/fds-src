/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.query.builder;

import com.formationds.commons.model.DateRange;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.persistence.EntityManager;
import javax.persistence.criteria.Path;
import javax.persistence.criteria.Predicate;
import java.util.ArrayList;

/**
 * @author ptinius
 */
@SuppressWarnings( "UnusedDeclaration" )
public class MetricCriteriaQueryBuilder
    extends CriteriaQueryBuilder<VolumeDatapoint> {

    private static final Logger logger =
        LoggerFactory.getLogger( MetricCriteriaQueryBuilder.class );

    private static final String TIMESTAMP = "timestamp";
    private static final String SERIES_TYPE = "key";
    private static final String CONTEXT = "volumeName";

    /**
     * @param entityManager the {@link javax.persistence.EntityManager}
     */
    public MetricCriteriaQueryBuilder( final EntityManager entityManager ) {
        super(entityManager, TIMESTAMP, CONTEXT);

        this.andPredicates = new ArrayList<>();
        this.orPredicates = new ArrayList<>();
    }

    /**
     * @param dateRange the {@link DateRange} representing the search date range window
     *
     * @return Returns {@link MetricCriteriaQueryBuilder}
     */
    @Override
    @SuppressWarnings( "unchecked" )
    public MetricCriteriaQueryBuilder withDateRange( final DateRange dateRange ) {
        if( dateRange != null ) {
            final Path<Long> timestamp = from.get( TIMESTAMP );

            Predicate predicate = null;
            if( ( dateRange.getStart() != null ) &&
                ( dateRange.getEnd() != null ) ) {
                predicate = cb.and( cb.ge( timestamp, dateRange.getStart() ),
                                    cb.le( timestamp, dateRange.getEnd() ) );
            } else if( dateRange.getStart() != null ) {
                predicate = cb.ge( from.<Long>get( TIMESTAMP ),
                                   dateRange.getStart() );
            } else if( dateRange.getEnd() != null ) {
                predicate = cb.le( from.<Long>get( TIMESTAMP ),
                                   dateRange.getEnd() );
            }

            if( predicate != null ) {
                andPredicates.add( predicate );
            }
        }

        return this;
    }

    /**
     * @param seriesType the {@link com.formationds.commons.model.type.Metrics}
     *
     * @return Returns {@link MetricCriteriaQueryBuilder}
     */
    public MetricCriteriaQueryBuilder withSeries( final Metrics seriesType ) {
        orPredicates.add( cb.equal( from.get( SERIES_TYPE ),
                                    seriesType.key() ) );
        return this;
    }
}
