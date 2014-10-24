/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.events.annotation;

import java.lang.annotation.*;

/**
 *
 */
@Documented
@Inherited
@Target( { ElementType.PARAMETER } )
@Retention( RetentionPolicy.RUNTIME )
public @interface EventArg {
    /**
     * The display name for the event argument.  If not specified, defaults to the
     * reflected parameter name.
     *
     * @return
     */
    String name() default "";

    /**
     * The hidden flag is used to explicitly identify sensitive arguments that must
     * not be exposed in messages in plaintext.
     *
     * @return true if the value for the argument must be hidden/encrypted in event messages
     */
    boolean hidden() default false;
}
