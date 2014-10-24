/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.entity;


import com.formationds.commons.events.EventCategory;
import com.formationds.commons.events.EventSeverity;
import com.formationds.commons.events.EventType;

import javax.persistence.Entity;
import javax.xml.bind.annotation.XmlRootElement;

@XmlRootElement
@Entity
public class SystemActivityEvent extends Event {

    protected SystemActivityEvent() {
    }

    public SystemActivityEvent(EventCategory category, EventSeverity severity, String messageKey, Object... messageArgs) {
        super(EventType.SYSTEM_EVENT, category, severity, messageKey, messageArgs);
    }
}
