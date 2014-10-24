/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.events.annotation;

import com.formationds.commons.events.AuditScope;

import java.lang.annotation.*;

/**
 * Annotation to mark a request handler implementation to be audited.
 * <p/>
 * The audit handler will check each request handler executed for the presence
 * of this annotation.  If present, the key and message arguments are used to
 * build a localizable audit event.
 */
@Documented
@Inherited
@Target( { ElementType.TYPE } )
@Retention( RetentionPolicy.RUNTIME )
public @interface Audit {

    /**
     * The message localization key to use.  If not explicitly provided, the
     * request handler class name is used as the message key.
     * <p/>
     * TODO: specifics of how message keys are looked up are TBD.
     *
     * @return the message localization key for the audit message
     */
    String key() default "";

    /**
     * TODO: since tied to request handlers, this may make sense to refer to
     * specific request properties or header field that require auditing.
     *
     * @return the message formatting arguments for the localized message key
     */
    String[] args() default {};

    /**
     * Specify the visibility of the event.  If not specified, the default scope is the object owner,
     * which implies tenant and global admin users.
     *
     * @return the visibility scope of the event
     */
    // TODO: should we make scoping explicit AuditScope[] ... default { GLOBAL_ADMIN, TENANT_ADMIN, OWNER } ?
    AuditScope visibility() default AuditScope.OWNER;
}
