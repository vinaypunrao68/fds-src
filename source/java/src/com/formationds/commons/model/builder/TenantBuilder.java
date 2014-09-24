/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.Tenant;
import com.formationds.commons.model.User;

import java.util.List;

/**
 * @author ptinius
 */
public class TenantBuilder {
  private long id;
  private String name;
  private List<User> users;

  /**
   * default constructor
   */
  public TenantBuilder() {
  }

  /**
   * @param id the {@code int} representing the tenant id
   *
   * @return Returns {@link com.formationds.commons.model.builder.TenantBuilder}
   */
  public TenantBuilder withId(long id) {
    this.id = id;
    return this;
  }

  /**
   * @param name the {@link String} representing the tenant name
   *
   * @return Returns {@link com.formationds.commons.model.builder.TenantBuilder}
   */
  public TenantBuilder withName(String name) {
    this.name = name;
    return this;
  }

  /**
   * @param users the {@link List} of {@link User}
   *
   * @return Returns {@link com.formationds.commons.model.builder.TenantBuilder}
   */
  public TenantBuilder withUsers(List<User> users) {
    this.users = users;
    return this;
  }

  /**
   * @return
   */
  public Tenant build() {
    Tenant tenant = new Tenant();
    tenant.setId(id);
    tenant.setName(name);
    tenant.setUsers(users);
    return tenant;
  }
}
