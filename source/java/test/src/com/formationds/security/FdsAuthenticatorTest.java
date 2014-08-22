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

    @Test(expected = LoginException.class)
    public void testMismatchedCredentials() throws Exception {
        ConfigurationService.Iface config = mock(ConfigurationService.Iface.class);
        when(config.allUsers(anyLong())).thenReturn(Lists.newArrayList());
        FdsAuthenticator module = new FdsAuthenticator(config);
        module.issueToken("fab", "fab");
    }

    @Test
    public void testSuccess() throws Exception {
        HashedPassword hasher = new HashedPassword();
        ConfigurationService.Iface config = mock(ConfigurationService.Iface.class);
        when(config.allUsers(anyLong())).thenReturn(                Lists.newArrayList(
                        new User(42, "james", hasher.hash("james"), "foo", false),
                        new User(43, "fab", hasher.hash("fab"), "bar", false)));

        FdsAuthenticator module = new FdsAuthenticator(config);
        AuthenticationToken token = module.issueToken("james", "james");
        assertEquals(42, token.getUserId());
        assertNotEquals("foo", token.getSecret());
        verify(config, times(1)).updateUser(eq(42l), eq("james"), anyString(), anyString(), eq(false));
    }
}