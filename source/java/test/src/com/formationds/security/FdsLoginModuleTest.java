package com.formationds.security;

import com.formationds.apis.User;
import com.formationds.om.CachedConfiguration;
import com.google.common.collect.Lists;
import com.google.common.collect.Maps;
import org.junit.Test;

import javax.security.auth.login.LoginException;
import java.util.Map;
import java.util.stream.Collectors;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

public class FdsLoginModuleTest {

    @Test(expected = LoginException.class)
    public void testLoginFails() throws Exception {
        CachedConfiguration config = mock(CachedConfiguration.class);
        when(config.usersByLogin()).thenReturn(Maps.newHashMap());
        FdsLoginModule module = new FdsLoginModule(config);
        module.login("fab", "fab");
    }

    @Test
    public void testLoginSucceeds() throws Exception {
        HashedPassword hasher = new HashedPassword();
        Map<String, User> map =
                Lists.newArrayList(
                        new User(0, "james", hasher.hash("james"), "foo", false),
                        new User(0, "fab", hasher.hash("fab"), "bar", false))
                .stream()
                .collect(Collectors.toMap(u -> u.getIdentifier(), u -> u));

        CachedConfiguration config = mock(CachedConfiguration.class);
        when(config.usersByLogin()).thenReturn(map);
        FdsLoginModule module = new FdsLoginModule(config);

        module.login("james", "james");
    }
}