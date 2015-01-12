package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.amazonaws.auth.AWSCredentials;
import com.amazonaws.auth.BasicAWSCredentials;
import com.formationds.security.AuthenticationToken;
import com.formationds.util.s3.S3SignatureGenerator;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;

import javax.crypto.SecretKey;
import java.text.MessageFormat;

public class S3Authenticator {
    private Xdi xdi;
    private SecretKey secretKey;

    public S3Authenticator(Xdi xdi, SecretKey secretKey) {
        this.xdi = xdi;
        this.secretKey = secretKey;
    }

    public AuthenticationToken authenticate(Request request) throws SecurityException {
        if (xdi.getAuthenticator().allowAll()) {
            return AuthenticationToken.ANONYMOUS;
        }

        String candidateHeader = request.getHeader("Authorization");
        AuthenticationComponents authenticationComponents = resolveFdsCredentials(candidateHeader);
        AWSCredentials basicAWSCredentials = new BasicAWSCredentials(authenticationComponents.principalName, authenticationComponents.fdsToken.signature(secretKey));

        String requestHash = S3SignatureGenerator.hash(request, basicAWSCredentials);

        if (candidateHeader.equals(requestHash)) {
            return authenticationComponents.fdsToken;
        } else {
            throw new SecurityException();
        }
    }

    private AuthenticationComponents resolveFdsCredentials(String header) {
        String pattern = "AWS {0}:{1}";
        Object[] parsed = new Object[0];
        try {
            parsed = new MessageFormat(pattern).parse(header);
            String principal = (String) parsed[0];
            AuthenticationToken fdsToken = xdi.getAuthenticator().currentToken(principal);
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

