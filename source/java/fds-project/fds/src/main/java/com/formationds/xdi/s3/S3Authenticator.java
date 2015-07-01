package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.amazonaws.auth.AWSCredentials;
import com.amazonaws.auth.BasicAWSCredentials;
import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.HttpContext;
import com.formationds.util.s3.auth.AuthenticationNormalizer;
import com.formationds.util.s3.auth.S3SignatureGeneratorV2;
import com.formationds.xdi.security.XdiAuthorizer;

import javax.crypto.SecretKey;
import javax.security.auth.login.LoginException;
import java.util.Optional;

public class S3Authenticator {
    private XdiAuthorizer authorizer;
    private SecretKey secretKey;
    private AuthenticationNormalizer normalizer;

    public S3Authenticator(XdiAuthorizer authorizer, SecretKey secretKey) {
        this.authorizer = authorizer;
        this.secretKey = secretKey;
        this.normalizer = new AuthenticationNormalizer();
    }

    public AuthenticationToken getIdentityClaim(HttpContext context) throws SecurityException {
        if (authorizer.allowAll())
            return AuthenticationToken.ANONYMOUS;

        if(normalizer.authenticationMode(context) == AuthenticationNormalizer.AuthenticationMode.Unknown)
            throw new SecurityException("unknown authentication scheme");

        Optional<String> principalName = normalizer.getPrincipalName(context);
        if(!principalName.isPresent())
            return AuthenticationToken.ANONYMOUS;

        return getAuthenticationToken(principalName.get());
    }

    private AuthenticationToken getAuthenticationToken(String principalName) {
        try {
            return authorizer.currentToken(principalName);
        } catch (LoginException e) {
            throw new SecurityException("invalid credentials");
        }
    }

    public HttpContext buildAuthenticatingContext(HttpContext context) {
        if (authorizer.allowAll())
            return context;

        Optional<String> principalName = normalizer.getPrincipalName(context);
        if(!principalName.isPresent())
            throw new SecurityException("no principal name found on context (or authentication scheme unknown), cannot build authenticating context");

        String principal = principalName.get();
        AuthenticationToken authenticationToken = getAuthenticationToken(principal);
        AWSCredentials basicAWSCredentials = new BasicAWSCredentials(principal, authenticationToken.signature(secretKey));
        return normalizer.authenticatingContext(basicAWSCredentials, context);
    }
}

