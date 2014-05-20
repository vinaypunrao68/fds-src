package com.formationds.security;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.sun.security.auth.UserPrincipal;

import javax.security.auth.Subject;
import javax.security.auth.callback.*;
import javax.security.auth.login.LoginException;
import javax.security.auth.spi.LoginModule;
import java.io.IOException;
import java.security.Principal;
import java.util.Map;

/*
 * Lifted from http://docs.oracle.com/javase/7/docs/technotes/guides/security/jaas/tutorials/SampleLoginModule.java
 */
public class NullLoginModule implements LoginModule {
    private Subject subject;
    private CallbackHandler callbackHandler;
    private Map<String, ?> sharedState;
    private Map<String, ?> options;
    private boolean loginSucceeded;
    private String username;
    private boolean commitSucceeded;
    private Principal principal;

    @Override
    public void initialize(Subject subject, CallbackHandler callbackHandler, Map<String, ?> sharedState, Map<String, ?> options) {
        this.subject = subject;
        this.callbackHandler = callbackHandler;
        this.sharedState = sharedState;
        this.options = options;
        loginSucceeded = false;
        commitSucceeded = false;
    }

    @Override
    public boolean login() throws LoginException {
        Callback[] callbacks = new Callback[2];
        callbacks[0] = new NameCallback("login");
        callbacks[1] = new PasswordCallback("password", false);

        try {
            callbackHandler.handle(callbacks);
        } catch (IOException e) {
            throw new LoginException(e.getMessage());
        } catch (UnsupportedCallbackException e) {
            throw new LoginException(e.getMessage());
        }
        username = ((NameCallback) callbacks[0]).getName();
        char[] password = ((PasswordCallback) callbacks[1]).getPassword();

        loginSucceeded = ("fds".equals(username) && "fds".equals(new String(password)));
        return loginSucceeded;
    }

    @Override
    public boolean commit() throws LoginException {
        if (loginSucceeded == false) {
            return false;
        }

        principal = new UserPrincipal(username);
        if (!subject.getPrincipals().contains(principal)) {
            subject.getPrincipals().add(principal);
        }
        username = null;
        commitSucceeded = true;
        return true;
    }

    @Override
    public boolean abort() throws LoginException {
        if (loginSucceeded == false) {
            return false;
        } else if (loginSucceeded && commitSucceeded) {
            username = null;
            principal = null;
        } else {
            logout();
        }

        return true;
    }

    @Override
    public boolean logout() throws LoginException {
        if (principal != null) {
            subject.getPrincipals().remove(principal);
        }
        loginSucceeded = false;
        commitSucceeded = false;
        username = null;
        principal = null;
        return true;
    }
}
