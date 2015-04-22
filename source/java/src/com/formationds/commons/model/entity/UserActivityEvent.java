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
public class UserActivityEvent extends Event {

    private Long userId;

    protected UserActivityEvent() {
    }

    /**
     *
     * @param userId
     * @param category event category
     * @param severity event severity
     * @param defaultMessageFmt default message format string
     * @param messageKey the message localization key
     * @param messageArgs the message format args
     */
    public UserActivityEvent(Long userId,
                             EventCategory category,
                             EventSeverity severity,
                             String defaultMessageFmt,
                             String messageKey,
                             Object... messageArgs) {
        super(EventType.USER_ACTIVITY, category, severity, defaultMessageFmt, messageKey, messageArgs);
        this.userId = userId;
    }

    /**
     *
     * @param ts the event timestamp
     * @param userId the event user id
     * @param category event category
     * @param severity event severity
     * @param defaultMessageFmt default message format string
     * @param messageKey the message localization key
     * @param messageArgs the message format args
     */
    public UserActivityEvent(Long ts,
                             Long userId,
                             EventCategory category,
                             EventSeverity severity,
                             String defaultMessageFmt,
                             String messageKey,
                             Object... messageArgs) {
        super(ts, EventType.USER_ACTIVITY, category, severity, defaultMessageFmt, messageKey, messageArgs);
        this.userId = userId;
    }

    /**
     * @return the user id
     */
    public Long getUserId() {
        return userId;
    }

    protected void setUserId(Long userId) {
        this.userId = userId;
    }
}
