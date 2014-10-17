/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.Series;
import com.formationds.commons.model.Statistics;
import com.formationds.commons.model.abs.Calculated;
import com.formationds.commons.model.abs.Metadata;

import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
public class StatisticsBuilder {
  private List<Series> series;
  private List<Calculated> calculated;
  private List<Metadata> metadata;

  public StatisticsBuilder() {
  }

  public StatisticsBuilder withSeries( List<Series> series ) {
    this.series = series;
    return this;
  }

  public StatisticsBuilder addSeries( Series series ) {
    if( this.series == null ) {
      this.series = new ArrayList<>();
    }

    this.series.add( series );
    return this;
  }

  public StatisticsBuilder withCalculated( List<Calculated> calculated ) {
    this.calculated = calculated;
    return this;
  }

  public StatisticsBuilder addCalculated( Calculated calculated ) {
    if( this.calculated == null ) {
      this.calculated = new ArrayList<>();
    }

    this.calculated.add( calculated );
    return this;
  }

  public StatisticsBuilder withMetadata( List<Metadata> metadata ) {
    this.metadata = metadata;
    return this;
  }

  public StatisticsBuilder addMetadata( Metadata metadata ) {
    if( this.metadata == null ) {
      this.metadata = new ArrayList<>();
    }

    this.metadata.add( metadata );
    return this;
  }

  public Statistics build() {
    Statistics statistics = new Statistics();

    if( series != null ) {
      statistics.setSeries( series );
    } else {
      statistics.setSeries( new ArrayList<>() );
    }

    if( calculated != null ) {
      statistics.setCalculated( calculated );
    } else {
      statistics.setCalculated( new ArrayList<>() );
    }

    if( metadata != null ) {
      statistics.setMetadata( metadata );
    } else {
      statistics.setMetadata( new ArrayList<>() );
    }

    return statistics;
  }
}
