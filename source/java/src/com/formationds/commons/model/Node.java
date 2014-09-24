/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;
import com.formationds.commons.model.type.NodeState;

import javax.xml.bind.annotation.XmlRootElement;
import java.beans.Transient;
import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
@XmlRootElement
public class Node
  extends ModelBase
{
  private static final long serialVersionUID = -6684434195412037211L;

  private int id = -1;
  private String name = null;
  private long uuid = -1L;

  private String site = null;
  private String domain = null;
  private String localDomain = null;
  private NodeState state = null;
  private String root = null;

  private long loAddr = -1L;
  private long hiAddr = -1L;

  private List<Service> services = null;

  /**
   * default package level constructor
   */
  public Node()
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

  /**
   * @return Returns {@code true} if the uuid is set
   */
  @Transient
  public boolean isUuid( ) { return getUuid() != -1L; }

  /**
   * @return Returns {@code true} if the uuid is set
   */
  @Transient
  public boolean isRoot( ) { return getRoot() != null; }

  /**
   * @return Returns {@code true} if the uuid is set
   */
  @Transient
  public boolean isService( ) { return services != null; }

  /**
   * @return Returns {@code true} if the node id is set
   */
  @Transient
  public boolean isId( ) { return getId() != -1; }

  /**
   * @return Returns {@code true} if the meta sync port is set
   */
  @Transient
  public boolean isLoAddr( ) { return isSet( getLoAddr() ); }

  /**
   * @return Returns {@code true} if the meta sync port is set
   */
  public boolean isHiAddr( ) { return isSet( getHiAddr() ); }

  /**
   * @return Returns {@code long} representing the universal unique identifier
   */
  public long getUuid()
  {
    return uuid;
  }

  /**
   * @param uuid the {@code long} representing the universal unique identifier
   */
  public void setUuid( final long uuid )
  {
    this.uuid = uuid;
  }

  /**
   * @return Returns {@link String} representing the domain
   */
  public String getDomain()
  {
    return domain;
  }

  /**
   * @param domain the {@link String} representing the domain
   */
  public void setDomain( final String domain )
  {
    this.domain = domain;
  }

  /**
   * @return Returns {@code long} representing the high order of the IP address
   */
  public long getHiAddr()
  {
    return hiAddr;
  }

  /**
   * @param hiAddr the {@code long} representing the high order of the IP address
   */
  public void setHiAddr( final long hiAddr )
  {
    this.hiAddr = hiAddr;
  }

  /**
   * @return Returns {@link String} representing identifier
   */
  public int getId()
  {
    return id;
  }

  /**
   * @param id the {@code int} representing the identifier
   */
  public void setId( final int id )
  {
    this.id = id;
  }

  /**
   * @return Returns {@code long} representing the low order of the IP address
   */
  public long getLoAddr()
  {
    return loAddr;
  }

  /**
   * @param loAddr the {@code long} representing the low order of the IP address
   */
  public void setLoAddr( final long loAddr )
  {
    this.loAddr = loAddr;
  }

  /**
   * @return Returns {@link String} representing the domain
   */
  public String getLocalDomain()
  {
    return localDomain;
  }

  /**
   * @param localDomain the {@link String} representing the domain
   */
  public void setLocalDomain( final String localDomain )
  {
    this.localDomain = localDomain;
  }

  /**
   * @return Returns {@link String} representing the name
   */
  public String getName()
  {
    return name;
  }

  /**
   * @param name the {@link String} representing the name
   */
  public void setName( final String name )
  {
    this.name = name;
  }

  /**
   * @return Returns {@link String} representing the root
   */
  public String getRoot()
  {
    return root;
  }

  /**
   * @param root the {@link String} representing the root
   */
  public void setRoot( final String root )
  {
    this.root = root;
  }

  /**
   * @return Returns {@link String} representing the site
   */
  public String getSite()
  {
    return site;
  }

  /**
   * @param site the {@link String} representing the site
   */
  public void setSite( final String site )
  {
    this.site = site;
  }

  /**
   * @return Returns {@link NodeState}
   */
  public NodeState getState()
  {
    return state;
  }

  /**
   * @param state the {@link NodeState}
   */
  public void setState( final NodeState state )
  {
    this.state = state;
  }

  /**
   * @return Returns {@code true} if the node is detached
   */
  public boolean isDetached()
  {
    /*
      Nate's javascript for determining if a node is attached or detached.

      if ( nodeService.node_type == 'FDSP_PLATFORM' &&
      nodeService.node_state == service.FDS_NODE_DISCOVERED ){
      tempDetached.push( nodeService );
      return;
      }
    */

    return getState().equals( NodeState.DISCOVERED );
  }

  /**
   * @param service the {@link Service}
   */
  public void addService( final Service service )
  {
    if( services == null )
    {
      services = new ArrayList<>( );
    }

    if( !services.contains( service ) )
    {
      services.add( service );
    }
  }

  /**
   * @return Returns {@link List} of {@link Service} that are installed on this node
   */
  public List<Service> getServices()
  {
    if( !isService() )
    {
      services = new ArrayList<>( );
    }

    return services;
  }
}
