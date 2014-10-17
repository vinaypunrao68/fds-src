/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.entity;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;

/**
 * @author ptinius
 */
public class QueryCriteriaValidator {
  private final Map<KeyFields, Boolean> required = new HashMap<>();

  private static final String MISSING_MANDATORY_CRITERIA =
    "Missing mandatory criteria field '%s'.";

  public enum KeyFields {
    VOLUME_NAME,
    DATE_RANGE,
    POINTS,
    FIRST_POINT,
    SERIES_TYPE,
    CONTEXT,
    RESULT_TYPE;
  }

  /**
   * default constructor
   */
  public QueryCriteriaValidator() {
  }

  public QueryCriteriaValidator with( final KeyFields field,
                                      final Boolean required ) {
    this.required.put( field, required );
    return this;
  }

  public void validate( final QueryCriteria criteria )
    throws IllegalArgumentException {
    final Set<KeyFields> keys = required.keySet();
    for( final KeyFields key : keys ) {
      switch( key.name() ) {
        case "DATE_RANGE":
          if( criteria == null || criteria.getRange() == null ) {
            throw new IllegalArgumentException( String.format( MISSING_MANDATORY_CRITERIA,
                                                               "date range" ) );
          }
          break;
        case "POINTS":
          if( criteria == null || criteria.getPoints() == null ) {
            throw new IllegalArgumentException( String.format( MISSING_MANDATORY_CRITERIA,
                                                               "max result set size" ) );
          }
          break;
        case "FIRST_POINT":
          if( criteria == null || criteria.getFirstPoint() == null ) {
            throw new IllegalArgumentException( String.format( MISSING_MANDATORY_CRITERIA,
                                                               "starting result set" ) );
          }
          break;
        case "SERIES_TYPE":
          if( criteria == null ||
            criteria.getSeriesType() == null ||
            criteria.getSeriesType()
                    .isEmpty() ) {
            throw new IllegalArgumentException( String.format( MISSING_MANDATORY_CRITERIA,
                                                               "series type" ) );
          }
          break;
        case "CONTEXT":
          if( criteria == null ||
            criteria.getContexts() == null ||
            criteria.getContexts()
                    .isEmpty() ) {
            throw new IllegalArgumentException( String.format( MISSING_MANDATORY_CRITERIA,
                                                               "context" ) );
          }
          break;
      }
    }
  }
}
