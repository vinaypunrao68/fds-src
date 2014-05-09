package com.formationds.security;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import javax.security.auth.callback.*;
import javax.security.auth.login.LoginContext;
import javax.security.auth.login.LoginException;
import java.io.IOException;

public class JaasAuthenticator implements Authenticator {
    @Override
    public boolean login(String login, String password) throws LoginException {
        LoginContext loginContext = new LoginContext("FDS", new CallbackHandler() {
            @Override
            public void handle(Callback[] callbacks) throws IOException, UnsupportedCallbackException {
                for (int i = 0; i < callbacks.length; i++) {
                    Callback callback = callbacks[i];

                    if (callback instanceof NameCallback) {
                        ((NameCallback) callback).setName(login);
                    } else if (callback instanceof PasswordCallback) {
                        ((PasswordCallback) callback).setPassword(password.toCharArray());
                    }
                }
            }
        });

        try {
            loginContext.login();
            return true;
        } catch (LoginException e) {
            return false;
        }
    }
}
