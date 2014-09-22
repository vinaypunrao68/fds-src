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
  private List<Feature> UserVisibleFeatures;

  /**
   * static utility method constructor
   */
  private AuthenticatedUserBuilder() {
  }

  /**
   * @return Returns a {@link AuthenticatedUserBuilder}
   */
  public static AuthenticatedUserBuilder anAuthenticatedUser() {
    return new AuthenticatedUserBuilder();
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
    this.UserVisibleFeatures = features;
    return this;
  }

  public AuthenticatedUserBuilder but() {
    return anAuthenticatedUser().withUsername( username )
                                .withUserId( userId )
                                .withToken( token )
                                .withFeatures( UserVisibleFeatures );
  }

  /**
   * @return Returns {@link AuthenticatedUserBuilder}
   */
  public AuthenticatedUser build() {
    AuthenticatedUser authenticatedUser = new AuthenticatedUser();
    authenticatedUser.setUsername( username );
    authenticatedUser.setUserId( userId );
    authenticatedUser.setToken( token );
    for( final Feature feature : UserVisibleFeatures ) {
      authenticatedUser.setFeatures( feature.getLocalized() );
    }

    return authenticatedUser;
  }
}
