/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.Context;
import com.formationds.commons.model.util.ModelFieldValidator;
import com.google.gson.annotations.SerializedName;

import java.util.HashMap;
import java.util.Map;
import java.util.Objects;

import static com.formationds.commons.model.util.ModelFieldValidator.KeyFields;
import static com.formationds.commons.model.util.ModelFieldValidator.outOfRange;

/**
 * @author ptinius
 */
public class Volume
  extends Context {
  private static final long serialVersionUID = 7961922641732546048L;

  private String name;
  private Long limit;                    // maximum IOPS
  private Long sla;                      // minimum IOPS -- service level agreement
  @SerializedName( "volumeId" )
  private String id;                     // volume Id
  private Integer priority;
  private Connector data_connector;
  private Usage current_usage;

  private static final Map<String, ModelFieldValidator> VALIDATORS =
    new HashMap<>();

  static {
    VALIDATORS.put( "priority",
                    new ModelFieldValidator( KeyFields.PRIORITY,
                                             PRIORITY_MIN,
                                             PRIORITY_MAX ) );
    VALIDATORS.put( "sla",
                    new ModelFieldValidator( KeyFields.SLA,
                                             SLA_MIN,
                                             SLA_MAX ) );
    VALIDATORS.put( "limit",
                    new ModelFieldValidator( KeyFields.LIMIT,
                                             SLA_MIN,
                                             SLA_MAX ) );
  }

  /**
   * default constructor
   */
  public Volume() {
    super();
  }

  @Override
  public boolean equals(Object o) {
    if (this == o) return true;
    if (o == null || getClass() != o.getClass()) return false;

    Volume volume = (Volume) o;

    if (!Objects.equals(id, volume.id)) return false;
    if (!Objects.equals(name, volume.name)) return false;

    return true;
  }

  @Override
  public int hashCode() {
    return Objects.hash(name, id);
  }

  /**
   * @return Returns a {@link String} representing the volume name
   */
  public String getName() {
    return name;
  }

  /**
   * @param name the {@link String} representing the volume name
   */
  public void setName( final String name ) {
    this.name = name;
  }

  /**
   * @return Returns a {@code int} representing the priority associated with
   * this volume
   */
  public int getPriority() {
    return priority;
  }

  /**
   * @param priority a {@code int} representing the priority associated with
   *                 this volume
   */
  public void setPriority( final int priority ) {
    if( !VALIDATORS.get( "priority" )
                   .isValid( priority ) ) {
      throw new IllegalArgumentException(
        outOfRange(
          VALIDATORS.get( "priority" ), ( long ) priority ) );
    }
    this.priority = priority;
  }

  /**
   * @return Returns a {@link String} representing the universally unique
   * identifier
   */
  public String getId() {
    return id;
  }

  /**
   * @param id a {@link String} representing the universally unique identifier
   */
  public void setId( final String id ) {
    this.id = id;
  }

  /**
   * @return Returns a {@code long} representing the minimum service level
   * agreement for IOPS
   */
  public long getSla() {
    return sla;
  }

  /**
   * @param sla a {@code long} representing the minimum service level agreement
   *            for IOPS
   */
  public void setSla( final long sla ) {
    if( !VALIDATORS.get( "sla" )
                   .isValid( sla ) ) {
      throw new IllegalArgumentException(
        outOfRange(
          VALIDATORS.get( "sla" ), sla ) );
    }

    this.sla = sla;
  }

  /**
   * @return Returns a {@code long} representing the maximum service level
   * agreement for IOPS
   */
  public long getLimit() {
    return limit;
  }

  /**
   * @param limit a {@code long} representing the maximum service level
   *              agreement for IOPS
   */
  public void setLimit( final long limit ) {
    if( !VALIDATORS.get( "limit" )
                   .isValid( limit ) ) {
      throw new IllegalArgumentException(
        outOfRange(
          VALIDATORS.get( "limit" ), limit ) );
    }

    this.limit = limit;
  }

  /**
   * @param data_connector a {@link Connector} representing type of data
   *                       connector
   */
  public void setData_connector( Connector data_connector ) {
    this.data_connector = data_connector;
  }

  /**
   * @return Returns a {@link Connector} representing the type of data
   * connector
   */
  public Connector getData_connector() {
    return data_connector;
  }

  /**
   * @return Returns the {@link Usage}
   */
  public Usage getCurrent_usage() {
    return current_usage;
  }

  /**
   * @param current_usage the {@link com.formationds.commons.model.Usage}
   */
  public void setCurrent_usage( final Usage current_usage ) {
    this.current_usage = current_usage;
  }

  /**
   * @return Returns a {@link T} representing the context
   */
  @SuppressWarnings( "unchecked" )
  @Override
  public <T> T getContext() {
    return ( T ) this.name;
  }
}
