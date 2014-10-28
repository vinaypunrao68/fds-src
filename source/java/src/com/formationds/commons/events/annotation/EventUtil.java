/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.events.annotation;

import com.formationds.commons.events.EventCategory;
import com.formationds.commons.events.EventSeverity;
import com.formationds.commons.events.EventType;
import com.formationds.commons.model.entity.*;

import java.lang.reflect.Method;
import java.lang.reflect.Parameter;
import java.util.ArrayList;
import java.util.List;

import com.formationds.security.AuthenticatedRequestContext;
import com.formationds.security.AuthenticationToken;
import org.slf4j.LoggerFactory;

/**
 * Event annotation utils.
 */
public class EventUtil {

    /**
     * Given a method and its arguments, process any Event annotations on the method and
     * generate an EventEntity with the associated event information from the annotation
     * and the method arguments.
     * <p/>
     * If no Event annotation is present, this method returns null indicating that no
     * event is generated from the method call.
     * <p/>
     * Event message arguments are determined based on @EventArg annotations on method parameter
     * declarations.
     *
     * @param m
     * @param args
     *
     * @return an Event entity with metadata specified by an associated Event annotation and the method args, or
     *  null if the method is not annotated.
     */
    public static com.formationds.commons.model.entity.Event toPersistentEvent(Method m, Object[] args) {
        // TODO: implement annotation util that can discover annotations on any implemented interfaces.
        Event ea = m.getAnnotation(Event.class);
        if (ea == null)
            return null;

        EventType t = ea.type();
        EventCategory c = ea.category();
        EventSeverity s = ea.severity();
        String key = ea.key();

        List<Object> eventArgs = new ArrayList<>(args.length);

        // EventArgs may be included in annotation body or on the parameters.
        EventArg[] eargs = ea.args();
        if (eargs == null) {
            eargs = new EventArg[m.getParameterCount()];
            Parameter[] params = m.getParameters();
            int i = 0;
            for (Parameter p : params) {
                EventArg earg = p.getAnnotation(EventArg.class);
                eargs[i++] = earg;
            }
        }

        // now extract argument values from method args
        for (int ai = 0; ai < eargs.length; ai++) {
            EventArg earg = eargs[ai];
            if (earg == null)
                continue;
            eventArgs.add(args[ai]);
        }

        switch (t) {
            case SYSTEM_EVENT:
                return new SystemActivityEvent(c, s, key, eventArgs.toArray());
            case USER_ACTIVITY:
                AuthenticationToken token = AuthenticatedRequestContext.getToken();
                return new UserActivityEvent(token.getUserId(), c, s, key, eventArgs.toArray());
            default:
                // TODO: log warning or throw exception on unknown/unsupported type?  Fow now logging.
                // throw new IllegalArgumentException("Unsupported event type (" + t + ")");
                LoggerFactory.getLogger(EventUtil.class).warn("Unsupported event type %s", t);
                return null;
        }
    }


}
