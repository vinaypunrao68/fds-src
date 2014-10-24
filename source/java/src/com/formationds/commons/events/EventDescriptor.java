/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.events;

import java.util.List;

public interface EventDescriptor {
    public EventType type();
    public EventCategory category();
    public EventSeverity severity();
    public String defaultMessage();
    public List<String> argNames();
    public String key();
}
