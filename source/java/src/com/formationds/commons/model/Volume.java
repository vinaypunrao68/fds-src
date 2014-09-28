/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;

import javax.xml.bind.annotation.XmlRootElement;

/**
 * @author ptinius
 */
@XmlRootElement
public class Volume
  extends ModelBase
{
  private static final long serialVersionUID = 7961922641732546048L;

/* JSON sent by the UI (CREATE)
{
  "name":"TestVolume",
  "data_connector":{
    "type":"Object",
    "api":"S3, Swift"
  },
  "priority":10,
  "sla":0,
  "limit":300,
  "snapshotPolicies":[
    {
      "name":"HOURLY",
      "recurrenceRule":{
        "frequency":"HOURLY"
      },
      "retention":14400
    }
  ]
}

JSON sent by Platform when listing volumes
[{
    "data_connector": {"type": "object"},
    "apis": "S3, Swift",
    "current_usage": {
        "unit": "B",
        "size": "0"
    },
    "name": "TestVolume",
    "limit": 300,
    "sla": 0,
    "id": "4565300393392428557",
    "priority": 10
}]

What I'm currently sending, this json (below) seems to cause the OM/DM to crash
{
  "name": "Volume Name -- 70579689",
  "limit": 1024,
  "sla": 256,
  "priority": 30,
  "data_connector": {
    "type": "OBJECT",
    "attributes": {
      "unit": "GB",
      "size": 100
    }
  },
  "current_usage": {
    "unit": "GB",
    "size": "100"
  },
  "apis": [
    "S3",
    "SWIFT"
  ]
}
*/

  private String name;
  private long limit;                    // maximum IOPS
  private long sla;                      // minimum IOPS -- service level agreement
  private String id;
  private int priority;
  private Connector data_connector;
  private Usage current_usage;

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
   * @return Returns a {@code long} representing the minimum service level agreement for IOPS
   */
  public long getSla()
  {
    return sla;
  }

  /**
   * @param sla a {@code long} representing the minimum service level agreement for IOPS
   */
  public void setSla( final long sla )
  {
    this.sla = sla;
  }

  /**
   * @return Returns a {@code long} representing the maximum service level agreement for IOPS
   */
  public long getLimit()
  {
    return limit;
  }

  /**
   * @param limit a {@code long} representing the maximum service level agreement for IOPS
   */
  public void setLimit( final long limit )
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
}
