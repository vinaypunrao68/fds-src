/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.stats;

import com.google.gson.annotations.SerializedName;

import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
public class Series {
  @SuppressWarnings("unused")
private static final long serialVersionUID = 8246474218117834832L;

  /*
   * use a series for the Statistics model object
   */

  @SerializedName("type")
  private String type;
  @SerializedName("context")
  private Object context;
  @SerializedName("datapoints")
  private List<Datapoint> datapoints;

  /**
   * @return Returns the {@link List} of {@link Datapoint}
   */
  public List<Datapoint> getDatapoints() {
    return datapoints;
  }

  /**
   * @param datapoints the {@link List} of {@link Datapoint}
   */
  public void setDatapoints( final List<Datapoint> datapoints ) {
    this.datapoints = datapoints;
  }

  /**
   * @param datapoint the {@link Datapoint}
   */
  public void setDatapoint( final Datapoint datapoint ) {
    if( this.datapoints == null ) {
      this.datapoints = new ArrayList<>();
    }

    this.datapoints.add( datapoint );
  }

  /**
   * @return Returns the {@link com.formationds.commons.model.Volume}
   */
  public Object getContext() {
    return context;
  }

  /**
   * @param volume the {@link com.formationds.commons.model.abs.Context}
   */
  public void setContext( final Object context ) {
    this.context = context;
  }

  /**
   * @return Returns the {@link String}
   */
  public String getType() {
    return type;
  }

  /**
   * @param type the {@link String}
   */
  public void setType( final String type ) {
    this.type = type;
  }
}
