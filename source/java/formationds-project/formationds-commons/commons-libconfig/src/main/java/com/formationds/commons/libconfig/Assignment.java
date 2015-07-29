package com.formationds.commons.libconfig;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Optional;

/**
 * If the specified {@code name} is not found, i.e. does not exists or is not
 * set the {@code value} attribute will be set to {@link
 * java.util.Optional#empty()}
 */
public class Assignment
    implements Node {

    private static final Logger logger =
        LoggerFactory.getLogger( Assignment.class );

    public static final String NOT_FOUND =
        "The specified name '%s' was not found or was not set.";

    private String name;
    private Optional value;

    Assignment( final String name, final Optional value ) {

        this.name = name;
        this.value = value;

    }

    public String getName() {
        return name;
    }

    public Optional getValue( ) {

        return value;

    }

    public int intValue() {

        return ( Integer ) instanceOf( value );

    }

    public String stringValue() {

        return ( String ) instanceOf( value );

    }

    public boolean booleanValue() {

        return ( Boolean ) instanceOf( value );

    }

    private Object instanceOf( final Optional value ) {

        if( value.isPresent() ) {

            if( value.get() instanceof String ) {

                return value.get();

            } else if( value.get() instanceof Integer ) {

                return value.get();

            } else if( value.get() instanceof Boolean ) {

                return value.get();

            }
        }

        final String s = String.format( NOT_FOUND, name );
        logger.warn( s );

        return String.format( "[ %s not set ]", name );
    }
}
