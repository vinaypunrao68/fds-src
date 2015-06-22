package com.formationds.security;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.User;

public class DumbAuthorizer implements Authorizer {
    @Override
    public long tenantId(AuthenticationToken token) throws SecurityException {
        return 0;
    }

  /**
   * @param token the authentication token
   * @param snapshotName the name of the snapshot
   * @return Returns {@code true} is snapshot is owned by; Otherwise {@code false}
   */
  @Override
  public boolean ownsSnapshot( final AuthenticationToken token,
                               final String snapshotName ) {
    return true;
  }

  @Override
    public boolean ownsVolume(AuthenticationToken token, String volume) {
        return true;
    }

    @Override
    public User userFor(AuthenticationToken token) throws SecurityException {
        return new User(0, "admin", "", "", true);
    }
}
