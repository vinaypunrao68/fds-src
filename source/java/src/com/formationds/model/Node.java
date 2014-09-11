/*
 * Copyright (C) 2014, All Rights Reserved, by Formation Data Systems, Inc.
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

package com.formationds.model;


import com.formationds.model.abs.ModelBase;
import com.formationds.model.type.ManagerType;
import com.formationds.model.type.NodeState;

import javax.xml.bind.annotation.XmlRootElement;
import java.beans.Transient;

/**
 * @author ptinius
 */
@XmlRootElement
public class Node
  extends ModelBase
{
  private static final long serialVersionUID = -1;

  private String name = null;
  private String site;
  private ManagerType type = null;
  private String svc_uuid = null;
  private String domain;
  private String localDomain;
  private NodeState state = null;
  private String uuid = null;
  private int id = -1;
  private String root = null;

  // ports
  private int controlPort = -1;
  private int dataPort = -1;
  private int migrationPort = -1;
  private int metasyncPort = -1;

  private long loAddr = -1L;
  private long hiAddr = -1L;

  /**
   * default constructor
   */
  Node()
  {
    super();
  }

  /**
   * @return Returns {@code true} if the name is set
   */
  @Transient
  public boolean isName( ) { return isSet( getName() ); }

  /**
   * @return Returns {@code true} if the site is set
   */
  @Transient
  public boolean isSite( ) { return isSet( getSite() ); }

  /**
   * @return Returns {@code true} if the type is set
   */
  @Transient
  public boolean isType( ) { return isSet( getType() ); }

  @Transient
  protected boolean isSet( final ManagerType type ) { return type != null; }

  /**
   * @return Returns {@code true} if the server uuid is set
   */
  @Transient
  public boolean isSvrUuid( ) { return isSet( getSvc_uuid() ); }

  /**
   * @return Returns {@code true} if the server uuid is set
   */
  @Transient
  public boolean isDomain( ) { return isSet( getDomain() ); }

  /**
   * @return Returns {@code true} if the server uuid is set
   */
  @Transient
  public boolean isLocalDomain( ) { return isSet( getLocalDomain() ); }

  /**
   * @return Returns {@code true} if the server uuid is set
   */
  @Transient
  public boolean isState( ) { return isSet( getState() ); }

  @Transient
  protected boolean isSet( final NodeState state ) { return state != null; };

  /**
   * @return Returns {@code true} if the uuid is set
   */
  @Transient
  public boolean isUuid( ) { return getUuid() != null; }

  /**
   * @return Returns {@code true} if the uuid is set
   */
  @Transient
  public boolean isRoot( ) { return getRoot() != null; }

  /**
   * @return Returns {@code true} if the node id is set
   */
  @Transient
  public boolean isId( ) { return getId() != -1; }

  /**
   * @return Returns {@code true} if the control port is set
   */
  @Transient
  public boolean isControlPort( ) { return getDataPort() != -1; }

  /**
   * @return Returns {@code true} if the control port is set
   */
  @Transient
  public boolean isDataPort( ) { return getControlPort() != -1; }

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

  /**
   * @return Returns {@code true} if the meta sync port is set
   */
  @Transient
  public boolean isLoAddr( ) { return isSet( getLoAddr() ); }

  /**
   * @return Returns {@code true} if the meta sync port is set
   */
  public boolean isHiAddr( ) { return isSet( getHiAddr() ); }

  public String getUuid()
  {
    return uuid;
  }

  public void setUuid( final String uuid )
  {
    this.uuid = uuid;
  }

  public int getControlPort()
  {
    return controlPort;
  }

  public void setControlPort( final int controlPort )
  {
    this.controlPort = controlPort;
  }

  public int getDataPort()
  {
    return dataPort;
  }

  public void setDataPort( final int dataPort )
  {
    this.dataPort = dataPort;
  }

  public String getDomain()
  {
    return domain;
  }

  public void setDomain( final String domain )
  {
    this.domain = domain;
  }

  public long getHiAddr()
  {
    return hiAddr;
  }

  public void setHiAddr( final long hiAddr )
  {
    this.hiAddr = hiAddr;
  }

  public int getId()
  {
    return id;
  }

  public void setId( final int id )
  {
    this.id = id;
  }

  public long getLoAddr()
  {
    return loAddr;
  }

  public void setLoAddr( final long loAddr )
  {
    this.loAddr = loAddr;
  }

  public String getLocalDomain()
  {
    return localDomain;
  }

  public void setLocalDomain( final String localDomain )
  {
    this.localDomain = localDomain;
  }

  public int getMetasyncPort()
  {
    return metasyncPort;
  }

  public void setMetasyncPort( final int metasyncPort )
  {
    this.metasyncPort = metasyncPort;
  }

  public int getMigrationPort()
  {
    return migrationPort;
  }

  public void setMigrationPort( final int migrationPort )
  {
    this.migrationPort = migrationPort;
  }

  public String getName()
  {
    return name;
  }

  public void setName( final String name )
  {
    this.name = name;
  }

  public String getRoot()
  {
    return root;
  }

  public void setRoot( final String root )
  {
    this.root = root;
  }

  public String getSite()
  {
    return site;
  }

  public void setSite( final String site )
  {
    this.site = site;
  }

  public NodeState getState()
  {
    return state;
  }

  public void setState( final NodeState state )
  {
    this.state = state;
  }

  public String getSvc_uuid()
  {
    return svc_uuid;
  }

  public void setSvc_uuid( final String svc_uuid )
  {
    this.svc_uuid = svc_uuid;
  }

  public ManagerType getType()
  {
    return type;
  }

  public void setType( final ManagerType type )
  {
    this.type = type;
  }
}
