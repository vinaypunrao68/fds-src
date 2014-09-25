/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.AuthenticatedUser;
import com.formationds.commons.model.type.Feature;

import java.util.List;

/**
 * @author ptinius
 */
public class AuthenticatedUserBuilder {
  private String username;
  private long userId;
  private String token;
  private List<Feature> features;

  /**
   * default constructor
   */
  public AuthenticatedUserBuilder() {
  }

  /**
   * @param username the user's name
   *
   * @return Returns {@link AuthenticatedUserBuilder}
   */
  public AuthenticatedUserBuilder withUsername( String username ) {
    this.username = username;
    return this;
  }

  /**
   * @param userId the user's id
   *
   * @return Returns {@link AuthenticatedUserBuilder}
   */
  public AuthenticatedUserBuilder withUserId( long userId ) {
    this.userId = userId;
    return this;
  }

  /**
   * @param token the system generated security token
   *
   * @return Returns {@link AuthenticatedUserBuilder}
   */
  public AuthenticatedUserBuilder withToken( String token ) {
    this.token = token;
    return this;
  }

  /**
   * @param features the {@link List} of {@link Feature} representation of the UI visible feature.
   *
   * @return Returns {@link AuthenticatedUserBuilder}
   */
  public AuthenticatedUserBuilder withFeatures( List<Feature> features ) {
    this.features = features;
    return this;
  }

  /**
   * @return Returns {@link AuthenticatedUserBuilder}
   */
  public AuthenticatedUser build() {
    AuthenticatedUser authenticatedUser = new AuthenticatedUser();
    authenticatedUser.setUsername( username );
    authenticatedUser.setUserId( userId );
    authenticatedUser.setToken( token );
    authenticatedUser.setFeatures( features );

    return authenticatedUser;
  }
}
