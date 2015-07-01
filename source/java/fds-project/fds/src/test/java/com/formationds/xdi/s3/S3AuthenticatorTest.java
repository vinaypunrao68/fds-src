package com.formationds.xdi.s3;

import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.HttpContext;
import com.formationds.xdi.security.XdiAuthorizer;
import org.junit.Test;

import javax.crypto.SecretKey;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

public class S3AuthenticatorTest {

    @Test
    public void testAnonymous() throws Exception {
        XdiAuthorizer authorizer = mock(XdiAuthorizer.class);
        when(authorizer.allowAll()).thenReturn(false);
        S3Authenticator s3Authenticator = new S3Authenticator(authorizer, mock(SecretKey.class));
        AuthenticationToken token = s3Authenticator.getIdentityClaim(mock(HttpContext.class));
        assertEquals(AuthenticationToken.ANONYMOUS, token);
    }
}