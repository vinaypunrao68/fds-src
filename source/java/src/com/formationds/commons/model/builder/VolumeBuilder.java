/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 *
 * This software is furnished under a license and may be used and copied only
 * in  accordance  with  the  terms  of such  license and with the inclusion
 * of the above copyright notice. This software or  any  other copies thereof
 * may not be provided or otherwise made available to any other person.
 * No title to and ownership of  the  software  is  hereby transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Formation Data Systems.
 *
 * Formation Data Systems assumes no responsibility for the use or  reliability
 * of its software on equipment which is not supplied by Formation Date Systems.
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
  private int limit;                    // maximum IOPS
  private int sla;                      // minimum IOPS
  private String id;
  private int priority;
  private Connector data_connector;
  private Usage current_usage;
  private String apis;

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
   * @param limit the {@code int} representing the iops limit
   *
   * @return Returns {@link com.formationds.commons.model.builder.VolumeBuilder}
   */
  public VolumeBuilder withLimit( int limit ) {
    this.limit = limit;
    return this;
  }

  /**
   * @param sla the {@code int} representing the service level agreement
   *
   * @return Returns {@link com.formationds.commons.model.builder.VolumeBuilder}
   */
  public VolumeBuilder withSla( int sla ) {
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
   * @param apis the {@link String} representing the data connector protocol
   *
   * @return Returns {@link com.formationds.commons.model.builder.VolumeBuilder}
   */
  public VolumeBuilder withApis( String apis ) {
    this.apis = apis;
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
