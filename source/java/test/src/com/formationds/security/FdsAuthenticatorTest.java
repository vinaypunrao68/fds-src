package com.formationds.security;

import com.formationds.apis.ConfigurationService;
import com.formationds.apis.User;
import com.formationds.util.thrift.ConfigurationApi;
import com.google.common.collect.Lists;
import org.junit.Before;
import org.junit.Test;

import javax.security.auth.login.LoginException;

import static org.junit.Assert.assertEquals;
import static org.mockito.AdditionalMatchers.not;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.*;

public class FdsAuthenticatorTest {

    public static final long USER_ID = 42;

    ConfigurationApi config;
    FdsAuthenticator authenticator;

    @Before
    public void setUp() throws Exception {
        config = mock(ConfigurationApi.class);
        authenticator = new FdsAuthenticator(config,
                                              AuthenticationTokenTest.SECRET_KEY);

    }

    @Test(expected = LoginException.class)
    public void testAuthenticateFailure() throws Exception {
        when(config.allUsers(anyLong())).thenReturn(Lists.newArrayList());
        authenticator.authenticate("foo", "bar");
    }

    @Test
    public void testAuthenticateSuccess() throws Exception {
        HashedPassword hasher = new HashedPassword();
        when(config.allUsers(anyLong())).thenReturn(Lists.newArrayList(
                new User(USER_ID, "james", hasher.hash("james"), "foo", false),
                new User(43, "fab", hasher.hash("fab"), "bar", false)));

        AuthenticationToken token = authenticator.authenticate("james", "james");
        assertEquals(42, token.getUserId());
    }

    @Test(expected = LoginException.class)
    public void testSignatureIntegrity() throws Exception {
        when(config.allUsers(anyLong())).thenReturn(Lists.newArrayList());
        authenticator.resolveToken("hello");
    }

    @Test(expected = LoginException.class)
    public void testOutDatedToken() throws Exception {
        String signature = new AuthenticationToken(USER_ID, "oldSecret").signature(AuthenticationTokenTest.SECRET_KEY);
        when(config.allUsers(anyLong())).thenReturn(Lists.newArrayList(new User(USER_ID, "james", "doesntMatter", "newSecret", false)));
        authenticator.resolveToken(signature);
    }

    @Test(expected = LoginException.class)
    public void testUserNoLongerExists() throws Exception {
        when(config.allUsers(anyLong())).thenReturn(Lists.newArrayList());
        String signature = new AuthenticationToken(USER_ID, "secret").signature(AuthenticationTokenTest.SECRET_KEY);
        authenticator.resolveToken(signature);
    }

    @Test
    public void testResolveToken() throws Exception {
        String secret = "secret";
        AuthenticationToken token = new AuthenticationToken(USER_ID, secret);
        when(config.allUsers(anyLong())).thenReturn(Lists.newArrayList(new User(USER_ID, "james", "doesntMatter", secret, false)));
        AuthenticationToken result = authenticator.resolveToken(token.signature(AuthenticationTokenTest.SECRET_KEY));
        assertEquals(token, result);
    }

    @Test
    public void testReissueToken() throws Exception {
        when(config.allUsers(anyLong())).thenReturn(Lists.newArrayList(
                new User(USER_ID, "james", "whatever", "oldSecret", false)));
        AuthenticationToken token = authenticator.reissueToken(USER_ID);
        verify(config, times(1)).updateUser(eq(USER_ID), eq("james"), eq("whatever"), not(eq("oldSecret")), eq(false));
    }
}