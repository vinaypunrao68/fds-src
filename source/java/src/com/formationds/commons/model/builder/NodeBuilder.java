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

import com.formationds.commons.model.Node;
import com.formationds.commons.model.type.NodeState;

/**
 * @author ptinius
 */
public class NodeBuilder {
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

  /**
   * default constructor
   */
  public NodeBuilder() {
  }

  /**
   * @return Returns the {@link NodeBuilder}
   */
  public NodeBuilder withId( int id ) {
    this.id = id;
    return this;
  }

  /**
   * @param name the {@link String} representing the name of the node
   *
   * @return Returns the {@link NodeBuilder}
   */
  public NodeBuilder withName( String name ) {
    this.name = name;
    return this;
  }

  /**
   * @param uuid teh {@code long} representing the universal unique identifier
   *
   * @return Returns the {@link NodeBuilder}
   */
  public NodeBuilder withUuid( long uuid ) {
    this.uuid = uuid;
    return this;
  }

  /**
   * @param site the {@link String} representing the site name
   *
   * @return Returns the {@link NodeBuilder}
   */
  public NodeBuilder withSite( String site ) {
    this.site = site;
    return this;
  }

  /**
   * @param domain the {@link String} representing the domain name
   *
   * @return Returns the {@link NodeBuilder}
   */
  public NodeBuilder withDomain( String domain ) {
    this.domain = domain;
    return this;
  }

  /**
   * @param localDomain the {@link String} representing the local domain
   *
   * @return Returns the {@link NodeBuilder}
   */
  public NodeBuilder withLocalDomain( String localDomain ) {
    this.localDomain = localDomain;
    return this;
  }

  /**
   * @param state the {@link NodeState}
   *
   * @return Returns the {@link NodeBuilder}
   */
  public NodeBuilder withState( NodeState state ) {
    this.state = state;
    return this;
  }

  /**
   * @param root the {@link String} representing the root
   *
   * @return Returns the {@link NodeBuilder}
   */
  public NodeBuilder withRoot( String root ) {
    this.root = root;
    return this;
  }

  /**
   * @param loAddr the {@code long} low-order address
   *
   * @return Returns the {@link NodeBuilder}
   */
  public NodeBuilder withLoAddr( long loAddr ) {
    this.loAddr = loAddr;
    return this;
  }

  /**
   * @param hiAddr the {@code long} high-order address
   *
   * @return Returns the {@link NodeBuilder}
   */
  public NodeBuilder withHiAddr( long hiAddr ) {
    this.hiAddr = hiAddr;
    return this;
  }

  /**
   * @return Returns the {@link Node}
   */
  public Node build() {
    Node node = new Node();
    node.setId( id );
    node.setName( name );
    node.setUuid( uuid );
    node.setSite( site );
    node.setDomain( domain );
    node.setLocalDomain( localDomain );
    node.setState( state );
    node.setRoot( root );
    node.setLoAddr( loAddr );
    node.setHiAddr( hiAddr );
    return node;
  }
}
