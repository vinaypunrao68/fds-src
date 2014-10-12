/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;
import com.google.gson.annotations.SerializedName;

import java.util.List;

/**
 * @author ptinius
 */
public class Series
extends ModelBase {
  private static final long serialVersionUID = 8246474218117834832L;

  @SerializedName( "context" )
  private Context context;
  @SerializedName( "datapoints" )
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
   * @return Returns the {@link com.formationds.commons.model.Volume}
   */
  public Context getContext() {
    return context;
  }

  /**
   * @param context the {@link com.formationds.commons.model.Context}
   */
  public void setContext( final Context context ) {
    this.context = context;
  }
}
