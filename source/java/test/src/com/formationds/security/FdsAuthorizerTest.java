package com.formationds.security;

import com.formationds.apis.User;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.util.thrift.ConfigurationApi;
import com.google.common.collect.Lists;
import org.apache.thrift.TException;
import org.junit.Before;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.*;

public class FdsAuthorizerTest {

    public static final String SECRET = "secret";
    public static final long USER_ID = 42;
    public static final User USER = new User(USER_ID, "james", "", SECRET, false);
    public static final String VOLUME_NAME = "foo";
    long tenantId = 13;

    private ConfigurationApi cachedConfig;
    private Authorizer authorizer;

    @Before
    public void setUp() throws Exception {
        cachedConfig = mock(ConfigurationApi.class);
        when(cachedConfig.getUser(eq(USER_ID))).thenReturn(USER);
        when(cachedConfig.allUsers(0)).thenReturn(Lists.newArrayList(USER));
        when(cachedConfig.tenantId(eq(USER_ID))).thenReturn(tenantId);
        authorizer = new FdsAuthorizer(cachedConfig);
    }

    @Test
    public void testUserForToken() throws Exception {
        User result = authorizer.userFor(new AuthenticationToken(USER_ID, SECRET));
        assertEquals(USER, result);
    }

    @Test(expected = SecurityException.class)
    public void testMismatchedSecret() {
        User result = authorizer.userFor(new AuthenticationToken(USER_ID, "badSecret"));
    }

    @Test
    public void testTenantId() {
        long result = authorizer.tenantId(new AuthenticationToken(USER_ID, SECRET));
        verify(cachedConfig, times(1)).tenantId(eq(USER_ID));
        assertEquals(tenantId, result);
    }

    @Test(expected = SecurityException.class)
    public void testNoTenant() {
        when(cachedConfig.tenantId(eq(USER_ID))).thenThrow(SecurityException.class);
        authorizer.tenantId(new AuthenticationToken(USER_ID, SECRET));
    }

    @Test(expected = SecurityException.class)
    public void testTenantIdWithSecretMismatch() {
        authorizer.tenantId(new AuthenticationToken(USER_ID, "badSecret"));
    }

    @Test(expected = SecurityException.class)
    public void testHasAccessWithSecretMismatch() {
        try {
            when(cachedConfig.statVolume("", "foo")).thenReturn(new VolumeDescriptor());
        } catch (TException e) {
            fail("unexpected exception." + e);
        }

        authorizer.hasAccess(new AuthenticationToken(USER_ID, "badSecret"), "foo");
    }

    @Test
    public void testHasAccessFails() {
        assertFalse(authorizer.hasAccess(new AuthenticationToken(USER_ID, SECRET), VOLUME_NAME));
    }

    @Test
    public void testHasAccessSucceeds() {
        assertFalse(authorizer.hasAccess(new AuthenticationToken(USER_ID, SECRET), "foo"));
    }
}