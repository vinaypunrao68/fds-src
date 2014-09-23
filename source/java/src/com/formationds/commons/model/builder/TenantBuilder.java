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
