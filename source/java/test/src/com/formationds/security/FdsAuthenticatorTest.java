package com.formationds.security;

import com.formationds.apis.ConfigurationService;
import com.formationds.apis.User;
import com.formationds.xdi.ConfigurationServiceCache;
import com.google.common.collect.Lists;
import org.junit.Test;

import javax.security.auth.login.LoginException;

import static org.junit.Assert.assertEquals;
import static org.mockito.AdditionalMatchers.not;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.*;

public class FdsAuthenticatorTest {

    public static final long USER_ID = 42;

    @Test(expected = LoginException.class)
    public void testAuthenticateFailure() throws Exception {
        ConfigurationService.Iface config = mock(ConfigurationService.Iface.class);
        when(config.allUsers(anyLong())).thenReturn(Lists.newArrayList());
        ConfigurationServiceCache cache = new ConfigurationServiceCache(config);
        FdsAuthenticator authenticator = new FdsAuthenticator(cache, AuthenticationTokenTest.SECRET_KEY);
        authenticator.authenticate("foo", "bar");
    }

    @Test
    public void testAuthenticateSuccess() throws Exception {
        HashedPassword hasher = new HashedPassword();
        ConfigurationService.Iface config = mock(ConfigurationService.Iface.class);
        when(config.allUsers(anyLong())).thenReturn(Lists.newArrayList(
                new User(USER_ID, "james", hasher.hash("james"), "foo", false),
                new User(43, "fab", hasher.hash("fab"), "bar", false)));

        ConfigurationServiceCache cache = new ConfigurationServiceCache(config);
        FdsAuthenticator authenticator = new FdsAuthenticator(cache, AuthenticationTokenTest.SECRET_KEY);
        AuthenticationToken token = authenticator.authenticate("james", "james");
        assertEquals(42, token.getUserId());
    }

    @Test(expected = LoginException.class)
    public void testSignatureIntegrity() throws Exception {
        ConfigurationServiceCache config = mock(ConfigurationServiceCache.class);
        FdsAuthenticator authenticator = new FdsAuthenticator(config, AuthenticationTokenTest.SECRET_KEY);
        authenticator.resolveToken("hello");
    }

    @Test(expected = LoginException.class)
    public void testOutDatedToken() throws Exception {
        String signature = new AuthenticationToken(USER_ID, "oldSecret").signature(AuthenticationTokenTest.SECRET_KEY);
        ConfigurationServiceCache config = mock(ConfigurationServiceCache.class);
        when(config.allUsers(anyLong())).thenReturn(Lists.newArrayList(new User(USER_ID, "james", "doesntMatter", "newSecret", false)));
        FdsAuthenticator authenticator = new FdsAuthenticator(config, AuthenticationTokenTest.SECRET_KEY);
        authenticator.resolveToken(signature);
    }

    @Test(expected = LoginException.class)
    public void testUserNoLongerExists() throws Exception {
        String signature = new AuthenticationToken(USER_ID, "secret").signature(AuthenticationTokenTest.SECRET_KEY);
        ConfigurationServiceCache config = mock(ConfigurationServiceCache.class);
        when(config.allUsers(anyLong())).thenReturn(Lists.newArrayList());
        FdsAuthenticator authenticator = new FdsAuthenticator(config, AuthenticationTokenTest.SECRET_KEY);
        authenticator.resolveToken(signature);
    }

    @Test
    public void testResolveToken() throws Exception {
        String secret = "secret";
        AuthenticationToken token = new AuthenticationToken(USER_ID, secret);
        ConfigurationServiceCache config = mock(ConfigurationServiceCache.class);
        when(config.allUsers(anyLong())).thenReturn(Lists.newArrayList(new User(USER_ID, "james", "doesntMatter", secret, false)));
        FdsAuthenticator authenticator = new FdsAuthenticator(config, AuthenticationTokenTest.SECRET_KEY);
        AuthenticationToken result = authenticator.resolveToken(token.signature(AuthenticationTokenTest.SECRET_KEY));
        assertEquals(token, result);
    }

    @Test
    public void testReissueToken() throws Exception {
        ConfigurationService.Iface config = mock(ConfigurationService.Iface.class);
        when(config.allUsers(anyLong())).thenReturn(Lists.newArrayList(
                new User(USER_ID, "james", "whatever", "oldSecret", false)));
        ConfigurationServiceCache poop = new ConfigurationServiceCache(config);
        FdsAuthenticator authenticator = new FdsAuthenticator(poop, AuthenticationTokenTest.SECRET_KEY);
        AuthenticationToken token = authenticator.reissueToken(USER_ID);
        verify(config, times(1)).updateUser(eq(USER_ID), eq("james"), eq("whatever"), not(eq("oldSecret")), eq(false));
    }
}