package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.amazonaws.auth.AWSCredentials;
import com.amazonaws.auth.BasicAWSCredentials;
import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.HttpContext;
import com.formationds.util.s3.S3SignatureGenerator;
import com.formationds.xdi.security.XdiAuthorizer;

import javax.crypto.SecretKey;
import java.text.MessageFormat;

public class S3Authenticator {
    private XdiAuthorizer authorizer;
    private SecretKey secretKey;

    public S3Authenticator(XdiAuthorizer authorizer, SecretKey secretKey) {
        this.authorizer = authorizer;
        this.secretKey = secretKey;
    }

    public AuthenticationToken authenticate(HttpContext context) throws SecurityException {
        if (authorizer.allowAll()) {
            return AuthenticationToken.ANONYMOUS;
        }

        String candidateHeader = context.getRequestHeader("Authorization");

        if (candidateHeader == null) {
            return AuthenticationToken.ANONYMOUS;
        }

        AuthenticationComponents authenticationComponents = resolveFdsCredentials(candidateHeader);
        AWSCredentials basicAWSCredentials = new BasicAWSCredentials(authenticationComponents.principalName, authenticationComponents.fdsToken.signature(secretKey));

        String requestHash = S3SignatureGenerator.hash(context, basicAWSCredentials);

        if (candidateHeader.equals(requestHash)) {
            return authenticationComponents.fdsToken;
        } else {
            throw new SecurityException();
        }
    }

    private AuthenticationComponents resolveFdsCredentials(String header) {
        String pattern = "AWS {0}:{1}";
        try {
            Object[] parsed = new MessageFormat(pattern).parse(header);
            String principal = (String) parsed[0];
            AuthenticationToken fdsToken = authorizer.currentToken(principal);
            return new AuthenticationComponents(principal, fdsToken);
        } catch (Exception e) {
            throw new SecurityException("invalid credentials");
        }
    }

    private class AuthenticationComponents {
        String principalName;
        AuthenticationToken fdsToken;

        AuthenticationComponents(String principalName, AuthenticationToken fdsToken) {
            this.principalName = principalName;
            this.fdsToken = fdsToken;
        }
    }
}

