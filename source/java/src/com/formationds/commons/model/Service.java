/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;
import com.formationds.commons.model.type.ManagerType;

import javax.xml.bind.annotation.XmlRootElement;
import java.beans.Transient;

/**
 * @author ptinius
 */
@XmlRootElement
public class Service
  extends ModelBase
{
  private static final long serialVersionUID = -1577170593630479004L;

  private long uuid = -1L;
  private int controlPort = -1;
  private int migrationPort = -1;
  private int dataPort = -1;
  private int metasyncPort = -1;
  private ManagerType type = ManagerType.UNKNOWN;

  /**
   * default package level constructor
   */
  public Service()
  {
    super();
  }

  /**
   * @return Returns {@code long} representing the service universal unique identifier
   */
  public long getUuid()
  {
    return uuid;
  }

  /**
   * @param uuid the service universal unique identifier
   */
  public void setUuid( final long uuid )
  {
    this.uuid = uuid;
  }

  /**
   * @return Returns {@link ManagerType}
   */
  public ManagerType getType()
  {
    return type;
  }

  /**
   * @param type the {@link ManagerType}
   */
  public void setType( final ManagerType type )
  {
    this.type = type;
  }

  /**
   * @return Returns {@code int} representing the listening port for data
   */
  public int getDataPort()
  {
    return dataPort;
  }

  /**
   * @param port the listening port for data
   */
  public void setDataPort( final int port )
  {
    this.dataPort = port;
  }

  /**
   * @return Returns {@code int} representing the listening port for migration
   */
  public int getMigrationPort()
  {
    return migrationPort;
  }

  /**
   * @param port the listening port for migration
   */
  public void setMigrationPort( final int port )
  {
    this.migrationPort = port;
  }

  /**
   * @return Returns {@code int} representing the listening port for control
   */
  public int getControlPort()
  {
    return controlPort;
  }

  /**
   * @param port the listening port for control
   */
  public void setControlPort( final int port )
  {
    this.controlPort = port;
  }

  /**
   * @return Returns {@code int} representing the listening port for meta sync
   */
  public int getMetasyncPort()
  {
    return metasyncPort;
  }

  /**
   * @param port the listening port for metadata sync
   */
  public void setMetasyncPort( final int port )
  {
    this.metasyncPort = port;
  }

  /**
   * @return Returns {@code true} if the server uuid is set
   */
  @Transient
  public boolean isSvrUuid( ) { return isSet( getUuid() ); }

  /**
   * @return Returns {@code true} if the type is set
   */
  @Transient
  public boolean isType( ) { return isSet( getType() ); }

  /**
   * @return Returns {@code true} if the control port is set
   */
  @Transient
  public boolean isControlPort( ) { return getControlPort() != -1; }

  /**
   * @return Returns {@code true} if the data port is set
   */
  @Transient
  public boolean isDataPort( ) { return getDataPort() != -1; }

  /**
   * @return Returns {@code true} if the migration port is set
   */
  @Transient
  public boolean isMigrationPort( ) { return getMigrationPort() != -1; }

  /**
   * @return Returns {@code true} if the meta sync port is set
   */
  @Transient
  public boolean isMetasyncPort( ) { return getMetasyncPort() != -1; }
}
