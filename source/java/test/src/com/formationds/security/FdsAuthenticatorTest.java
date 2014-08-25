package com.formationds.security;

import com.formationds.apis.ConfigurationService;
import com.formationds.apis.User;
import com.google.common.collect.Lists;
import org.junit.Test;

import javax.security.auth.login.LoginException;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Mockito.*;

public class FdsAuthenticatorTest {

    public static final int USER_ID = 42;

    @Test(expected = LoginException.class)
    public void testReissueTokenFailure() throws Exception {
        ConfigurationService.Iface config = mock(ConfigurationService.Iface.class);
        when(config.allUsers(anyLong())).thenReturn(Lists.newArrayList());
        FdsAuthenticator authenticator = new FdsAuthenticator(config);
        authenticator.reissueToken("fab", "fab");
    }

    @Test
    public void testReissueTokenSuccess() throws Exception {
        HashedPassword hasher = new HashedPassword();
        ConfigurationService.Iface config = mock(ConfigurationService.Iface.class);
        when(config.allUsers(anyLong())).thenReturn(                Lists.newArrayList(
                        new User(42, "james", hasher.hash("james"), "foo", false),
                        new User(43, "fab", hasher.hash("fab"), "bar", false)));

        FdsAuthenticator authenticator = new FdsAuthenticator(config);
        AuthenticationToken token = authenticator.reissueToken("james", "james");
        assertEquals(42, token.getUserId());
        assertNotEquals("foo", token.getSecret());
        verify(config, times(1)).updateUser(eq(42l), eq("james"), anyString(), anyString(), eq(false));
    }

    @Test(expected = LoginException.class)
    public void testSignatureIntegrity() throws Exception {
        ConfigurationService.Iface config = mock(ConfigurationService.Iface.class);
        FdsAuthenticator authenticator = new FdsAuthenticator(config);
        authenticator.resolveToken("hello");
    }

    @Test(expected = LoginException.class)
    public void testOutDatedToken() throws Exception {
        String signature = new AuthenticationToken(USER_ID, "oldSecret").signature(AuthenticationTokenTest.SECRET_KEY);
        ConfigurationService.Iface config = mock(ConfigurationService.Iface.class);
        when(config.allUsers(anyLong())).thenReturn(Lists.newArrayList(new User(USER_ID, "james", "doesntMatter", "newSecret", false)));
        FdsAuthenticator authenticator = new FdsAuthenticator(config);
        authenticator.resolveToken(signature);
    }

    @Test(expected = LoginException.class)
    public void testUserNoLongerExists() throws Exception {
        String signature = new AuthenticationToken(USER_ID, "secret").signature(AuthenticationTokenTest.SECRET_KEY);
        ConfigurationService.Iface config = mock(ConfigurationService.Iface.class);
        when(config.allUsers(anyLong())).thenReturn(Lists.newArrayList());
        FdsAuthenticator authenticator = new FdsAuthenticator(config);
        authenticator.resolveToken(signature);
    }

        @Test
    public void testResolveToken() throws Exception {
        String secret = "secret";
        AuthenticationToken token = new AuthenticationToken(USER_ID, secret);
        ConfigurationService.Iface config = mock(ConfigurationService.Iface.class);
        when(config.allUsers(anyLong())).thenReturn(Lists.newArrayList(new User(USER_ID, "james", "doesntMatter", secret, false)));
        FdsAuthenticator authenticator = new FdsAuthenticator(config);
        AuthenticationToken result = authenticator.resolveToken(token.signature(AuthenticationTokenTest.SECRET_KEY));
        assertEquals(token, result);
    }
}