/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;

import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
public class  Statistics
  extends ModelBase {
  private static final long serialVersionUID = 1565568840222449672L;

  private List<Series> series;
  private List<Calculated> calculated;
  private List<Metadata> metadata;

  /**
   * default constructor
   */
  public Statistics() {
  }

  public List<Series> getSeries() {
    return series;
  }

  public void setSeries( final List<Series> series ) {
    this.series = series;
  }

  public void addSeries( final Series series ) {
    if( this.series == null ) {
      this.series = new ArrayList<>( );
    }

    this.series.add( series );
  }

  public List<Calculated> getCalculated() {
    return calculated;
  }

  public void setCalculated( final List<Calculated> calculated ) {
    this.calculated = calculated;
  }

  public void addCalculated( final Calculated calculated ) {
    if( this.calculated == null ) {
      this.calculated = new ArrayList<>( );
    }

    this.calculated.add( calculated );
  }

  public List<Metadata> getMetadata() {
    return metadata;
  }

  public void setMetadata( final List<Metadata> metadata ) {
    this.metadata = metadata;
  }

  public void addMetadata( final Metadata metadata ) {
    if( this.metadata == null ) {
      this.metadata = new ArrayList<>( );
    }
    this.metadata.add( metadata );
  }
}
