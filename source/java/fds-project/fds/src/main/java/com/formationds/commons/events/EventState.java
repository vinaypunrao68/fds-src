/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.events;

/**
 * TODO: do we want to represent state similar to Nagios
 * where there is a SOFT (detected problem, but give it some time to re-check)
 * and a HARD state type indicating configured re-check attempts have all failed.
 *
 * http://nagios.sourceforge.net/docs/nagioscore/4/en/statetypes.html
 */
public enum EventState {
    SOFT, HARD, RECOVERED
}
