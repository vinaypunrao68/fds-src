/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.Context;
import com.formationds.commons.model.abs.ModelBase;
import com.formationds.commons.model.type.Metrics;
import com.google.gson.annotations.SerializedName;

import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
public class Series
  extends ModelBase {
  private static final long serialVersionUID = 8246474218117834832L;

  /*
   * use a series for the Statistics model object
   */

  @SerializedName("type")
  private Metrics type;
  @SerializedName("context")
  private Context context;
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
  public Context getContext() {
    return context;
  }

  /**
   * @param context the {@link com.formationds.commons.model.abs.Context}
   */
  public void setContext( final Context context ) {
    this.context = context;
  }

  /**
   * @return Returns the {@link com.formationds.commons.model.type.Metrics}
   */
  public Metrics getType() {
    return type;
  }

  /**
   * @param type the {@link com.formationds.commons.model.type.Metrics}
   */
  public void setType( final Metrics type ) {
    this.type = type;
  }
}
