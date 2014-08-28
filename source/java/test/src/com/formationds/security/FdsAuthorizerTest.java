package com.formationds.security;

import com.formationds.apis.User;
import com.formationds.xdi.CachedConfiguration;
import com.google.common.collect.Lists;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.*;

public class FdsAuthorizerTest {

    public static final String SECRET = "secret";
    public static final long USER_ID = 42;
    public static final User USER = new User(USER_ID, "james", "", SECRET, false);
    public static final String VOLUME_NAME = "foo";

    @Test
    public void testUserForToken() {
        CachedConfiguration cachedConfig = mock(CachedConfiguration.class);
        when(cachedConfig.users()).thenReturn(Lists.newArrayList(USER));

        Authorizer authorizer = new FdsAuthorizer(() -> cachedConfig);
        User result = authorizer.userFor(new AuthenticationToken(USER_ID, SECRET));
        assertEquals(USER, result);
    }

    @Test(expected = SecurityException.class)
    public void testMismatchedSecret() {
        CachedConfiguration cachedConfig = mock(CachedConfiguration.class);
        when(cachedConfig.users()).thenReturn(Lists.newArrayList(USER));
        Authorizer authorizer = new FdsAuthorizer(() -> cachedConfig);
        User result = authorizer.userFor(new AuthenticationToken(USER_ID, "badSecret"));
    }

    @Test
    public void testTenantId() {
        CachedConfiguration cachedConfig = mock(CachedConfiguration.class);
        long tenantId = 13;
        when(cachedConfig.users()).thenReturn(Lists.newArrayList(USER));
        when(cachedConfig.tenantId(eq(USER_ID))).thenReturn(tenantId);
        Authorizer authorizer = new FdsAuthorizer(() -> cachedConfig);
        long result = authorizer.tenantId(new AuthenticationToken(USER_ID, SECRET));
        verify(cachedConfig, times(1)).tenantId(eq(USER_ID));
        assertEquals(tenantId, result);
    }

    @Test(expected = SecurityException.class)
    public void testNoTenant() {
        CachedConfiguration cachedConfig = mock(CachedConfiguration.class);
        when(cachedConfig.tenantId(eq(USER_ID))).thenThrow(SecurityException.class);
        Authorizer authorizer = new FdsAuthorizer(() -> cachedConfig);
        authorizer.tenantId(new AuthenticationToken(USER_ID, SECRET));
    }

    @Test(expected = SecurityException.class)
    public void testTenantIdWithSecretMismatch() {
        CachedConfiguration cachedConfig = mock(CachedConfiguration.class);
        long tenantId = 13;
        when(cachedConfig.tenantId(eq(USER_ID))).thenReturn(tenantId);
        Authorizer authorizer = new FdsAuthorizer(() -> cachedConfig);
        authorizer.tenantId(new AuthenticationToken(USER_ID, "badSecret"));
    }

    @Test(expected = SecurityException.class)
    public void testHasAccessWithSecretMismatch() {
        CachedConfiguration cachedConfig = mock(CachedConfiguration.class);
        when(cachedConfig.users()).thenReturn(Lists.newArrayList(USER));

        Authorizer authorizer = new FdsAuthorizer(() -> cachedConfig);
        authorizer.hasAccess(new AuthenticationToken(USER_ID, "badSecret"), "foo");
    }

    @Test
    public void testHasAccessFails() {
        CachedConfiguration cachedConfig = mock(CachedConfiguration.class);
        when(cachedConfig.users()).thenReturn(Lists.newArrayList(USER));
        when(cachedConfig.hasAccess(eq(USER_ID), eq(VOLUME_NAME))).thenReturn(false);
        Authorizer authorizer = new FdsAuthorizer(() -> cachedConfig);
        assertFalse(authorizer.hasAccess(new AuthenticationToken(USER_ID, SECRET), VOLUME_NAME));
        verify(cachedConfig, times(1)).hasAccess(eq(USER_ID), eq(VOLUME_NAME));
    }

    @Test
    public void testHasAccessSucceeds() {
        CachedConfiguration cachedConfig = mock(CachedConfiguration.class);
        when(cachedConfig.users()).thenReturn(Lists.newArrayList(USER));
        when(cachedConfig.hasAccess(eq(USER_ID), eq(VOLUME_NAME))).thenReturn(false);
        Authorizer authorizer = new FdsAuthorizer(() -> cachedConfig);
        assertFalse(authorizer.hasAccess(new AuthenticationToken(USER_ID, SECRET), "foo"));
        verify(cachedConfig, times(1)).hasAccess(eq(USER_ID), eq(VOLUME_NAME));
    }

}