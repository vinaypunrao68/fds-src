/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
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
