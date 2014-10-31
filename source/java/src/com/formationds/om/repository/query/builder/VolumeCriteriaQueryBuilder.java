/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.query.builder;

import com.formationds.commons.model.DateRange;
import com.formationds.commons.model.abs.Context;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.om.repository.query.QueryCriteria;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.persistence.EntityManager;
import javax.persistence.TypedQuery;
import javax.persistence.criteria.*;
import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
@SuppressWarnings("UnusedDeclaration")
public class VolumeCriteriaQueryBuilder extends CriteriaQueryBuilder<VolumeDatapoint> {
    private static final Logger logger =
        LoggerFactory.getLogger( VolumeCriteriaQueryBuilder.class );

    private static final String SERIES_TYPE = "key";
    private static final String CONTEXT = "volumeName";

    /**
     * @param entityManager the {@link javax.persistence.EntityManager}
     */
    public VolumeCriteriaQueryBuilder( final EntityManager entityManager ) {
        super(entityManager, CONTEXT);
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
}
