package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.security.Principal;

public class XdiCredentials {
    private Principal principal;
    private AuthenticationKey authenticationKey;

    public XdiCredentials(Principal principal, AuthenticationKey authenticationKey) {
        this.principal = principal;
        this.authenticationKey = authenticationKey;
    }

    public Principal getPrincipal() {
        return principal;
    }

    public AuthenticationKey getAuthenticationKey() {
        return authenticationKey;
    }
}
