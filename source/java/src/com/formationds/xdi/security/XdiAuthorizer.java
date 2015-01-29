package com.formationds.xdi.security;

import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authenticator;
import com.formationds.security.Authorizer;

import javax.security.auth.login.LoginException;

public class XdiAuthorizer {
    private Authenticator authenticator;
    private Authorizer authorizer;

    public XdiAuthorizer(Authenticator authenticator, Authorizer authorizer) {
        this.authenticator = authenticator;
        this.authorizer = authorizer;
    }

    public boolean hasVolumePermission(AuthenticationToken token, String volume, Intent intent) {
        return authorizer.ownsVolume(token, volume);
    }

    public boolean hasToplevelPermission(AuthenticationToken token, Intent intent) {
        return !token.equals(AuthenticationToken.ANONYMOUS);
    }

    public long tenantId(AuthenticationToken token) {
        return authorizer.tenantId(token);
    }

    public boolean allowAll() {
        return authenticator.allowAll();
    }

    public AuthenticationToken currentToken(String principal) throws LoginException {
        return authenticator.currentToken(principal);
    }

    public AuthenticationToken authenticate(String login, String password) throws LoginException {
        return authenticator.authenticate(login, password);
    }

    public AuthenticationToken parseToken(String tokenHeader) throws LoginException {
        return authenticator.parseToken(tokenHeader);
    }
}
