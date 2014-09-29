/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;

import javax.xml.bind.annotation.XmlRootElement;
import java.util.List;

/**
 * @author ptinius
 */
@XmlRootElement
public class Tenant
  extends ModelBase {
  private static final long serialVersionUID = -1159983056189923654L;

  private Long id;
  private String name;
  private List<User> users;

  /**
   * default constructor
   */
  public Tenant() {
    super();
  }

  /**
   * @return Returns {@code long} representing the tenant id
   */
  public Long getId() {
    return id;
  }

  /**
   * @param id the {@code long} representing the tenant id
   */
  public void setId( final Long id ) {
    this.id = id;
  }

  /**
   * @return Returns {@link List} of {@link com.formationds.commons.model.User}
   */
  public List<User> getUsers() {
    return users;
  }

  /**
   * @param users the {@link List} of {@link com.formationds.commons.model.User}
   */
  public void setUsers( final List<User> users ) {
    this.users = users;
  }

  /**
   * @return Returns {@link String} representing the tenants name
   */
  public String getName() {
    return name;
  }

  /**
   * @param name the {@link String} representing the tenants name
   */
  public void setName( final String name ) {
    this.name = name;
  }
}
