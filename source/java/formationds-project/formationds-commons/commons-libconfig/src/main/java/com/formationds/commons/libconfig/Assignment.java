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
    private String value;

    Assignment(final String name, final String value) {
        this.name = name;
        this.value = value == null ? "" : value.replaceAll("^\"", "").replaceAll("\"$", "");
    }

    public String getName() {
        return name;
    }

    public Optional<String> getValue() {
        if (!value.isEmpty()) {
            return Optional.of(value);
        } else {
            return Optional.empty();
        }
    }

    public int intValue() {
        return Integer.parseInt(value);
    }

    public String stringValue() {
        return value;
    }

    public boolean booleanValue() {
        return Boolean.parseBoolean(value);
    }

    public long longValue() {
        return Long.parseLong(value);
    }
}
