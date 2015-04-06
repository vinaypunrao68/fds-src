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

    /**
     * A system event using now as a timestamp.
     *
     * @param category event category
     * @param severity event severity
     * @param defaultMessageFmt default message format string
     * @param messageKey the message localization key
     * @param messageArgs the message format args
     */
    public SystemActivityEvent(EventCategory category,
                               EventSeverity severity,
                               String defaultMessageFmt,
                               String messageKey,
                               Object... messageArgs) {
        super(EventType.SYSTEM_EVENT, category, severity, defaultMessageFmt, messageKey, messageArgs);
    }

    /**
     *
     * @param ts the event timestamp
     * @param category event category
     * @param severity event severity
     * @param defaultMessageFmt default message format string
     * @param messageKey the message localization key
     * @param messageArgs the message format args
     */
    public SystemActivityEvent(Long ts,
                               EventCategory category,
                               EventSeverity severity,
                               String defaultMessageFmt,
                               String messageKey,
                               Object... messageArgs) {
        super(ts, EventType.SYSTEM_EVENT, category, severity, defaultMessageFmt, messageKey, messageArgs);
    }

}
