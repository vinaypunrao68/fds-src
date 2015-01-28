package com.formationds.xdi.s3;

import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authenticator;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;
import org.junit.Test;

import javax.crypto.SecretKey;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

public class S3AuthenticatorTest {

    @Test
    public void testAnonymous() throws Exception {
        Xdi xdi = mock(Xdi.class);
        Authenticator authenticator = mock(Authenticator.class);
        when(xdi.getAuthenticator()).thenReturn(authenticator);
        when(authenticator.allowAll()).thenReturn(false);
        S3Authenticator s3Authenticator = new S3Authenticator(xdi, mock(SecretKey.class));
        AuthenticationToken token = s3Authenticator.authenticate(mock(Request.class));
        assertEquals(AuthenticationToken.ANONYMOUS, token);
    }
}