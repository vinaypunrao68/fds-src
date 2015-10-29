package com.formationds.om.webkit.rest;

import com.formationds.apis.User;
import com.formationds.om.webkit.rest.v07.users.UpdatePassword;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.AuthenticationTokenTest;
import com.formationds.security.Authorizer;
import com.formationds.web.toolkit.Resource;
import com.formationds.util.thrift.ConfigurationApi;
import com.google.common.collect.Lists;
import org.junit.Before;
import org.junit.Test;

import javax.servlet.http.HttpServletResponse;

import static org.junit.Assert.assertEquals;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.*;

public class UpdatePasswordTest {
    private static final User ADMIN = new User(0, "admin", "poop", "secret", true);
    private static final User FAB = new User(1, "fab", "poop", "secret", false);
    private static final User PANDA = new User(2, "panda", "poop", "secret", false);

    private ConfigurationApi cache;

    @Test
    public void adminCanChangeAnyPassword() throws Exception {
        AuthenticationToken authenticationToken = new AuthenticationToken(ADMIN.getId(), "foo");
        Authorizer authorizer = mock(Authorizer.class);
        when(authorizer.userFor(authenticationToken)).thenReturn(ADMIN);

        UpdatePassword action = new UpdatePassword(authenticationToken, cache, AuthenticationTokenTest.SECRET_KEY, authorizer);
        Resource resource = action.execute(ADMIN, FAB.getId(), "foo");
        verify(cache, times(1)).updateUser(eq(FAB.getId()), eq("fab"), anyString(), anyString(), eq(false));
        assertEquals(HttpServletResponse.SC_OK, resource.getHttpStatus());
    }


    @Test
    public void usersCanChangeTheirOwnPassword() throws Exception {
        AuthenticationToken authenticationToken = new AuthenticationToken(FAB.getId(), "foo");
        Authorizer authorizer = mock(Authorizer.class);
        when(authorizer.userFor(authenticationToken)).thenReturn(FAB);

        UpdatePassword action = new UpdatePassword(authenticationToken, cache, AuthenticationTokenTest.SECRET_KEY, authorizer);
        Resource resource = action.execute(FAB, FAB.getId(), "foo");
        verify(cache, times(1)).updateUser(eq(FAB.getId()), eq("fab"), anyString(), anyString(), eq(false));
        assertEquals(HttpServletResponse.SC_OK, resource.getHttpStatus());
    }

    @Test
    public void usersCannotChangeAnotherUsersPassword() throws Exception {
        AuthenticationToken authenticationToken = new AuthenticationToken(FAB.getId(), "foo");
        Authorizer authorizer = mock(Authorizer.class);
        when(authorizer.userFor(authenticationToken)).thenReturn(FAB);
        UpdatePassword action = new UpdatePassword(authenticationToken, cache, AuthenticationTokenTest.SECRET_KEY, authorizer);
        Resource resource = action.execute(FAB, PANDA.getId(), "foo");
        verify(cache, times(0)).updateUser(anyLong(), anyString(), anyString(), anyString(), anyBoolean());
        assertEquals(HttpServletResponse.SC_UNAUTHORIZED, resource.getHttpStatus());
    }

    @Before
    public void setUp() throws Exception {
        cache = mock(ConfigurationApi.class);
        when(cache.allUsers(0)).thenReturn(Lists.newArrayList(
                ADMIN,
                FAB,
                PANDA
        ));
    }
}
