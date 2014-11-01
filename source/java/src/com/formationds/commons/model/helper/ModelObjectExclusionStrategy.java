/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.helper;

import com.google.gson.ExclusionStrategy;
import com.google.gson.FieldAttributes;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * @author ptinius
 */
public class ModelObjectExclusionStrategy
    implements ExclusionStrategy {

    private final Map<Class<?>,List<String>> excludeFields = new HashMap<>( );
    private final List<Class<?>> excludeClasses = new ArrayList<>( );

    /**
     * @param f the field object that is under test
     *
     * @return true if the field should be ignored; otherwise false
     */
    @Override
    public boolean shouldSkipField( final FieldAttributes f ) {
        return false;
    }

    /**
     * @param fqfn the {@link String} representing the full qualified field name
     *
     * @throws ClassNotFoundException if the {@link Class} is not found
     */
    public void excludeField( final String fqfn )
        throws ClassNotFoundException {
        final Class<?> clazz =
            Class.forName( fqfn.substring( 0, fqfn.lastIndexOf( "." ) ) );

        final String fieldName = fqfn.substring( fqfn.lastIndexOf( "." ) + 1 );
        if( !excludeFields.containsKey( clazz ) ) {
            excludeFields.put( clazz, new ArrayList<>( ) );
        }

        excludeFields.get( clazz ).add( fieldName );
    }

    /**
     * @param clazz the class object that is under test
     *
     * @return true if the class should be ignored; otherwise false
     */
    @Override
    public boolean shouldSkipClass( final Class<?> clazz ) {
        return excludeClasses.contains( clazz );
    }

    /**
     * @param clazz the {@link String} representing the full qualified class name
     */
    public void excludeClass( final Class<?> clazz ) {

        excludeClasses.add( clazz );
    }
}
