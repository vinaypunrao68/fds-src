/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;
import com.formationds.commons.model.type.Feature;

import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
public class AuthenticatedUser
  extends ModelBase {
  private static final long serialVersionUID = -6030735171650443376L;

  private String username;
  private Long userId;
  private String token;
  private List<Feature> features;

  /**
   * default constructor
   */
  public AuthenticatedUser() {
    super();
  }

  /**
   * @return Returns {@link String} representing the user name
   */
  public String getUsername() {
    return username;
  }

  /**
   * @param username the {@link String} representing the user name
   */
  public void setUsername( final String username ) {
    this.username = username;
  }

  /**
   * @return Returns {@link Long} representing the user's is
   */
  public Long getUserId() {
    return userId;
  }

  /**
   * @param userId the {@code long} representing the user's id
   */
  public void setUserId( final Long userId ) {
    this.userId = userId;
  }

  /**
   * @return Returns {@link String} representing the authentication
   */
  public String getToken() {
    return token;
  }

  /**
   * @param token the {@link String} representing the authentication
   */
  public void setToken( final String token ) {
    this.token = token;
  }

  /**
   * @return Returns {@link List} a {@link Feature} representation of the UI
   * visible feature.
   */
  public List<Feature> getFeatures() {
    if( features == null ) {
      features = new ArrayList<>();
    }

    return features;
  }

  /**
   * @param features the {@link List} of {@link String} representation of the
   *                 UI visible feature.
   */
  public void setFeatures( final List<Feature> features ) {
    if( this.features == null ) {
      this.features = new ArrayList<>();
    }

    this.features.addAll( features );
  }
}
