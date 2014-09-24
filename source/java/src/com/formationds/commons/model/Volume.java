/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;
import com.formationds.commons.model.type.Protocol;

import javax.xml.bind.annotation.XmlRootElement;
import java.util.List;

/**
 * @author ptinius
 */
@XmlRootElement
public class Volume
  extends ModelBase
{
  private static final long serialVersionUID = 7961922641732546048L;

  private String name;
  private int limit;                    // maximum IOPS
  private int sla;                      // minimum IOPS -- service level agreement
  private String id;
  private int priority;
  private Connector data_connector;
  private Usage current_usage;
  private List<Protocol> apis;

  /**
   * default constructor
   */
  public Volume()
  {
    super();
  }

  /**
   * @return Returns a {@link String} representing the volume name
   */
  public String getName()
  {
    return name;
  }

  /**
   * @param name the {@link String} representing the volume name
   */
  public void setName( final String name )
  {
    this.name = name;
  }

  /**
   * @return Returns a {@code int} representing the priority associated with this volume
   */
  public int getPriority()
  {
    return priority;
  }

  /**
   * @param priority a {@code int} representing the priority associated with this volume
   */
  public void setPriority( final int priority )
  {
    this.priority = priority;
  }

  /**
   * @return Returns a {@link String} representing the universally unique identifier
   */
  public String getId()
  {
    return id;
  }

  /**
   * @param id a {@link String} representing the universally unique identifier
   */
  public void setId( final String id )
  {
    this.id = id;
  }

  /**
   * @return Returns a {@code int} representing the minimum service level agreement for IOPS
   */
  public int getSla()
  {
    return sla;
  }

  /**
   * @param sla a {@code int} representing the minimum service level agreement for IOPS
   */
  public void setSla( final int sla )
  {
    this.sla = sla;
  }

  /**
   * @return Returns a {@code int} representing the maximum service level agreement for IOPS
   */
  public int getLimit()
  {
    return limit;
  }

  /**
   * @param limit a {@code int} representing the maximum service level agreement for IOPS
   */
  public void setLimit( final int limit )
  {
    this.limit = limit;
  }

  /**
   * @param data_connector a {@link Connector} representing type of data connector
   */
  public void setData_connector( Connector data_connector )
  {
    this.data_connector = data_connector;
  }

  /**
   * @return Returns a {@link Connector} representing the type of data connector
   */
  public Connector getData_connector()
  {
    return data_connector;
  }

  /**
   * @return Returns the {@link Usage}
   */
  public Usage getCurrent_usage()
  {
    return current_usage;
  }

  /**
   * @param current_usage the {@link com.formationds.commons.model.Usage}
   */
  public void setCurrent_usage( final Usage current_usage )
  {
    this.current_usage = current_usage;
  }

  /**
   * @return Returns {@link String} representing the data connector protocol
   */
  public List<Protocol> getApis() {
    return apis;
  }

  /**
   * @param apis the {@link String} representing the data connector protocol
   */
  public void setApis( final String apis ) {
    this.apis = Protocol.lookup( apis );
  }
}
