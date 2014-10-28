/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.security;

/**
 * Basic authentication token tracking over a request.
 * <p/>
 * Usage: <br/>
 * <code>
 *     AuthenticatedToken token ...
 *     AuthenticatedRequestContext.begin(token)
 *     try {
 *         // execute the request.
 *     }
 *     finally {
 *         AuthenticatedRequestContext.complete();
 *     }
 * </code>
 *
 * This has some implementation considerations to be aware of:
 * <ul>
 *     <li>It will not work reliably if requests are split into multiple sub-tasks that execute in a thread-pool.
 *     The expectation is that it is accessed in code that executes on the same thread that manages the context.</li>
 *     <li>Access must follow the try/finally model in the same way required for java.util.concurrent.lock.ReentrantLock
 *     access, as illustrated above.  Otherwise, it may be susceptible to causing memory leaks over time.</li>
 * </ul>
 */
public enum AuthenticatedRequestContext {

    /* This class is used to track AuthenticationTokens over a request so that the event management
     * code can access the token necessary to obtain the user id associated with the request.
     * The code was placed here because the alternative of storing it in the HttpAuthenticator
     * created a circular dependency from the events code (in commons) package, and the Xdi package.
     *
     * There may be alternative solutions in Java 8 lambda support that make this approach obsolete.  This is
     * the minimum necessary to access the auth token for beta 1 and may changee.
     */


    INSTANCE;

    private static final ThreadLocal<AuthenticationToken> AUTH_SESSION = new InheritableThreadLocal<>();

    /**
     * Begin an authenticated request
     * @param t
     */
    public static void begin(AuthenticationToken t) { AUTH_SESSION.set(t); }

    /**
     * Complete an authenticated request
     */
    public static void complete() { AUTH_SESSION.remove(); }

    /**
     *
     * @return the current threads authentication token, or null if not authenticated.
     */
    public static AuthenticationToken getToken() { return AUTH_SESSION.get(); }
}
