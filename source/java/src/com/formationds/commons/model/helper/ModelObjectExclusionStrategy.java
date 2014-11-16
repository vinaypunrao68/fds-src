/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.helper;

import com.google.gson.ExclusionStrategy;
import com.google.gson.FieldAttributes;

import java.lang.reflect.Field;
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
        return excludeFields.get( f.getDeclaredClass() )
                            .contains( f.getName() );
    }

    /**
     * @param field the {@link java.lang.reflect.Field} representing the field
     *              name to be excluded
     *
     * @throws ClassNotFoundException if the {@link Class} is not found
     */
    public void excludeField( final Field field )
        throws ClassNotFoundException {
        final Class<?> clazz = field.getDeclaringClass();

        if( !excludeFields.containsKey( clazz ) ) {
            excludeFields.put( clazz, new ArrayList<>( ) );
        }

        excludeFields.get( clazz ).add( field.getName() );
    }

    /**
     * @param clazz the {@link Class} representing the {@code clazz} to be excluded.
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
