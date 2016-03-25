/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
package com.formationds.demo;

import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.LogManager;

public class Stoppable implements Runnable {
    private static final Logger LOG = LogManager.getLogger(Stoppable.class);

    private String name;
    private Action action;
    private boolean shouldRun;

    public static interface Action {
        void execute() throws Exception;
    }

    protected Stoppable(String name, Action action) {
        this.name = name;
        this.action = action;
        shouldRun = true;
    }

    @Override
    public void run() {
        while (shouldRun) {
            try {
                action.execute();
                LOG.debug("Stoppable '" + name + "' successful");
            } catch (InterruptedException e) {
                LOG.debug("Stoppable '" + name + "' exiting");
                break;
            } catch (Exception e) {
                LOG.error("Stoppable '" + name + "' got an exception", e);
            }
        }
    }
}
