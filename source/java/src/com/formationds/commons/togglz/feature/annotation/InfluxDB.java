/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.togglz.feature.annotation;

import org.togglz.core.annotation.FeatureGroup;
import org.togglz.core.annotation.Label;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 *
 * @author davefds
 */
@FeatureGroup
@Label( "InfluxDB Feature Group" )
@Target( ElementType.FIELD )
@Retention( RetentionPolicy.RUNTIME )
public @interface InfluxDB
{
    // no content, this is the influx feature annotation
}

