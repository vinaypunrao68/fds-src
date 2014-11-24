/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.commons.events;

import java.io.Serializable;

/**
 * Supported event types
 */
public enum EventType implements Serializable {
    USER_ACTIVITY,
    SYSTEM_EVENT,
    FIREBREAK_EVENT;
}
