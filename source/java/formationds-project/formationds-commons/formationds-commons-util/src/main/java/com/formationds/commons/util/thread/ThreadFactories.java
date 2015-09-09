/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.commons.util.thread;

import java.lang.Thread.UncaughtExceptionHandler;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.atomic.AtomicInteger;

public class ThreadFactories {

    /**
     * @param prefix
     * @return a new ThreadFactory that creates new daemon threads with the specified prefix and sequential counter.
     */
    public static ThreadFactory newThreadFactory( String prefix ) {
      return newThreadFactory( null,
                               prefix,
                               true,
                               Thread.NORM_PRIORITY,
                               Thread.getDefaultUncaughtExceptionHandler() );
    }

    /**
     * @param prefix
     * @param daemon
     * @return a new thread factory that creates new threads with the specified prefix
     */
    public static ThreadFactory newThreadFactory( String prefix,
                                                  boolean daemon ) {
        return newThreadFactory( null,
                                 prefix,
                                 daemon,
                                 Thread.NORM_PRIORITY,
                                 Thread.getDefaultUncaughtExceptionHandler() );
    }

    /**
     *
     * @param group
     * @param prefix
     * @param daemon
     * @param priority
     * @param uncaughtExceptionHandler
     * @return a new thread factory that creates new threads with the specified prefix
     */
    public static ThreadFactory newThreadFactory( ThreadGroup group,
                                                  String prefix,
                                                  boolean daemon,
                                                  int priority,
                                                  UncaughtExceptionHandler uncaughtExceptionHandler ) {
        final AtomicInteger seq = new AtomicInteger( 1 );
        return (r) -> {
            String threadName = String.format( "%s-%d", prefix, seq.getAndIncrement() );
            final Thread t = new Thread( group, r, threadName );
            t.setDaemon( daemon );
            if ( t.getPriority() != priority ) {
                t.setPriority( priority );
            }
            if (uncaughtExceptionHandler != null ) {
                t.setUncaughtExceptionHandler( uncaughtExceptionHandler );
            }
            return t;
        };
    }
}
