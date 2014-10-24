/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.entity;


import com.formationds.commons.events.EventCategory;
import com.formationds.commons.events.EventSeverity;
import com.formationds.commons.events.EventType;
import com.formationds.commons.model.User;

import javax.persistence.Entity;
import javax.xml.bind.annotation.XmlRootElement;

@XmlRootElement
@Entity
public class UserActivityEvent extends Event {

    private Long userId;

    protected UserActivityEvent() {
    }

    public UserActivityEvent(Long userId, EventCategory category, EventSeverity severity, String messageKey, Object... messageArgs) {
        super(EventType.USER_ACTIVITY, category, severity, messageKey, messageArgs);
        this.userId = userId;
    }
}
