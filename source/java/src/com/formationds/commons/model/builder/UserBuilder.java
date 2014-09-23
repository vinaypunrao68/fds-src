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

import com.formationds.commons.model.User;
import com.formationds.commons.model.type.IdentityType;

/**
 * @author ptinius
 */
public class UserBuilder {
  private long id;
  private IdentityType identifier;
  private String passwordHash;
  private String secret;

  /**
   * default constructor
   */
  public UserBuilder() {
  }

  /**
   * @param id the {@code int} representing the user's id
   *
   * @return Returns {@link com.formationds.commons.model.builder.UserBuilder}
   */
  public UserBuilder withId(final long id) {
    this.id = id;
    return this;
  }

  /**
   * @param identifier the {@link com.formationds.commons.model.type.IdentityType} representing the type of user
   *
   * @return Returns {@link com.formationds.commons.model.builder.UserBuilder}
   */
  public UserBuilder withIdentifier(final IdentityType identifier) {
    this.identifier = identifier;
    return this;
  }

  /**
   * @param passwordHash the {@link String} representing user's hashed password
   *
   * @return Returns {@link com.formationds.commons.model.builder.UserBuilder}
   */
  public UserBuilder withPasswordHash(final String passwordHash) {
    this.passwordHash = passwordHash;
    return this;
  }

  /**
   * @param secret the {@code int} representing the user's id
   *
   * @return Returns {@link com.formationds.commons.model.builder.UserBuilder}
   */
  public UserBuilder withSecret(final String secret) {
    this.secret = secret;
    return this;
  }

  /**
   * @return Returns {@link com.formationds.commons.model.User}
   */
  public User build() {
    User user = new User();
    user.setId(id);
    user.setIdentifier(identifier);
    user.setPasswordHash(passwordHash);
    user.setSecret(secret);
    return user;
  }
}
