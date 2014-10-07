/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.Connector;
import com.formationds.commons.model.Usage;
import com.formationds.commons.model.Volume;

/**
 * @author ptinius
 */
public class VolumeBuilder {
  private String name;
  private Long limit;                    // maximum IOPS
  private Long sla;                      // minimum IOPS
  private String id;
  private Integer priority;
  private Connector data_connector;
  private Usage current_usage;

  /**
   * default constructor
   */
  public VolumeBuilder() {
  }

  /**
   * @param name the {@link String} representing the volume.
   *
   * @return Returns {@link com.formationds.commons.model.builder.VolumeBuilder}
   */
  public VolumeBuilder withName( String name ) {
    this.name = name;
    return this;
  }

  /**
   * @param limit the {@code long} representing the iops limit
   *
   * @return Returns {@link com.formationds.commons.model.builder.VolumeBuilder}
   */
  public VolumeBuilder withLimit( long limit ) {
    this.limit = limit;
    return this;
  }

  /**
   * @param sla the {@code int} representing the service level agreement
   *
   * @return Returns {@link com.formationds.commons.model.builder.VolumeBuilder}
   */
  public VolumeBuilder withSla( long sla ) {
    this.sla = sla;
    return this;
  }

  /**
   * @param id the {@link String} representing volume id
   *
   * @return Returns {@link com.formationds.commons.model.builder.VolumeBuilder}
   */
  public VolumeBuilder withId( String id ) {
    this.id = id;
    return this;
  }

  /**
   * @param priority the {@code int} representing the priority
   *
   * @return Returns {@link com.formationds.commons.model.builder.VolumeBuilder}
   */
  public VolumeBuilder withPriority( int priority ) {
    this.priority = priority;
    return this;
  }

  /**
   * @param data_connector the {@link Connector} representing the data connector
   *
   * @return Returns {@link com.formationds.commons.model.builder.VolumeBuilder}
   */
  public VolumeBuilder withData_connector( Connector data_connector ) {
    this.data_connector = data_connector;
    return this;
  }

  /**
   * @param current_usage the {@link Usage} representing current usage
   *
   * @return Returns {@link com.formationds.commons.model.builder.VolumeBuilder}
   */
  public VolumeBuilder withCurrent_usage( Usage current_usage ) {
    this.current_usage = current_usage;
    return this;
  }

  /**
   * @return Returns {@link com.formationds.commons.model.Volume}
   */
  public Volume build() {
    Volume volume = new Volume();
    volume.setName( name );
    volume.setLimit( limit );
    volume.setSla( sla );
    volume.setId( id );
    volume.setPriority( priority );
    volume.setData_connector( data_connector );
    volume.setCurrent_usage( current_usage );
    return volume;
  }
}
