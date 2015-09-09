/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.stats;

import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
public class Statistics {
  @SuppressWarnings("unused")
private static final long serialVersionUID = 1565568840222449672L;

  private List<Series> series;
  private List<Calculated> calculated;    // de-dup ratio/% consumer/etc.
  private List<Metadata> metadata;        // query details

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
