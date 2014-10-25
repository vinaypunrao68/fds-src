/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.Datapoint;
import com.formationds.commons.model.Series;
import com.formationds.commons.model.abs.Context;
import com.formationds.commons.model.type.Metrics;

import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
public class SeriesBuilder {
  private Context context;
  private Metrics type;
  private List<Datapoint> datapoints;

  /**
   * default constructor
   */
  public SeriesBuilder() {
  }

  /**
   * @param context the {@link Context}
   *
   * @return Returns the {@link com.formationds.commons.model.builder.SeriesBuilder}
   */
  public SeriesBuilder withContext( Context context ) {
    this.context = context;
    return this;
  }

  /**
   * @param datapoints the {@link List} of {@link com.formationds.commons.model.Datapoint}
   *
   * @return Returns the {@link com.formationds.commons.model.builder.SeriesBuilder}
   */
  public SeriesBuilder withDatapoints( List<Datapoint> datapoints ) {
    this.datapoints = datapoints;
    return this;
  }

  /**
   * @param datapoint the {@link com.formationds.commons.model.Datapoint}
   *
   * @return Returns the {@link com.formationds.commons.model.builder.SeriesBuilder}
   */
  public SeriesBuilder withDatapoint( Datapoint datapoint ) {
    if( datapoints == null ) {
      this.datapoints = new ArrayList<>();
    }

    this.datapoints.add( datapoint );
    return this;
  }

  /**
   * @param type the {@link com.formationds.commons.model.type.Metrics} type
   *
   * @return Returns the {@link com.formationds.commons.model.builder.SeriesBuilder}
   */
  public SeriesBuilder withType( final Metrics type ) {
    this.type = type;
    return this;
  }

  /**
   * @return Returns the {@link com.formationds.commons.model.Series}
   */
  public Series build() {
    Series volumeDatapointSeries = new Series();

    if( context != null ) {
      volumeDatapointSeries.setContext( context );
    }

    if( datapoints != null ) {
      volumeDatapointSeries.setDatapoints( datapoints );
    } else {
      volumeDatapointSeries.setDatapoints( new ArrayList<>() );
    }

    if( type != null ) {
      volumeDatapointSeries.setType( type );
    }

    return volumeDatapointSeries;
  }
}
