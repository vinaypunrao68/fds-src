/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;

/**
 * @author ptinius
 */
public class SystemCapability
    extends ModelBase {

    private static final long serialVersionUID = 6201198734284264206L;

    private String[] supportedMediaPolicies;

    public SystemCapability( final String[] supportedMediaPolicies ) {

        this.supportedMediaPolicies = supportedMediaPolicies;

    }

    public String[] getSupportedMediaPolicies( ) {
        return supportedMediaPolicies;
    }
}
