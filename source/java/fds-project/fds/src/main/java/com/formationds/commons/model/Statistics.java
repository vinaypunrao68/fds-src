/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.Calculated;
import com.formationds.commons.model.abs.Metadata;
import com.formationds.commons.model.abs.ModelBase;
import com.formationds.om.repository.query.QueryCriteria;

import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
public class Statistics
  extends ModelBase {
  private static final long serialVersionUID = 1565568840222449672L;

  private List<Series> series;
  private List<Calculated> calculated;    // de-dup ratio/% consumer/etc.
  private List<Metadata> metadata;        // query details
  private QueryCriteria query;

  /**
   * default constructor
   */
  public Statistics() {
  }

  /**
   * @return Returns the {@link List} of {@link Series}
   */
  public List<Series> getSeries() {
    if( this.series == null ) {
      this.series = new ArrayList<>( );
    }

    return series;
  }

  /**
   * @param series adds a {@link java.util.Collections} of {@link Series}
   */
  public void setSeries( final List<Series> series ) {
    this.series = series;
  }

  /**
   * @param series adds a single {@link Series}
   */
  public void addSeries( final Series series ) {
    if( this.series == null ) {
      this.series = new ArrayList<>();
    }

    this.series.add( series );
  }
  
  /**
   * 
   * @param query add the query into the stats return {@link QueryCriteria}
   */
  public void setQuery( QueryCriteria query ){
	  this.query = query;
  }
  
  /**
   * 
   * @return retrieve the query that was used to generate these statistics {@link QueryCriteria}
   */
  public QueryCriteria getQuery(){
	  return this.query;
  }

  /**
   * @return Returns the {@link List} of {@link Calculated}
   */
  public List<Calculated> getCalculated() {
    if( this.calculated == null ) {
      this.calculated = new ArrayList<>( );
    }

    return calculated;
  }

  /**
   * @param calculated adds a {@link java.util.List} of {@link Calculated}
   */
  public void setCalculated( final List<Calculated> calculated ) {
    this.calculated = calculated;
  }

  /**
   * @param calculated adds a single {@link Calculated}
   */
  public void addCalculated( final Calculated calculated ) {
    if( this.calculated == null ) {
      this.calculated = new ArrayList<>();
    }

    this.calculated.add( calculated );
  }

  /**
   * @return Returns the {@link List} of {@link Metadata}
   */
  public List<Metadata> getMetadata() {
    if( this.metadata == null ) {
      this.metadata = new ArrayList<>( );
    }

    return metadata;
  }

  /**
   * @param metadata adds a {@link java.util.List} of {@link Metadata}
   */
  public void setMetadata( final List<Metadata> metadata ) {
    this.metadata = metadata;
  }

  /**
   * @param metadata adds a single {@link Metadata}
   */
  public void addMetadata( final Metadata metadata ) {
    if( this.metadata == null ) {
      this.metadata = new ArrayList<>();
    }
    this.metadata.add( metadata );
  }
}
