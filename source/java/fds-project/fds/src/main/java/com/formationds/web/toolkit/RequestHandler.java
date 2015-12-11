package com.formationds.web.toolkit;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.eclipse.jetty.server.Request;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Map;
import java.util.Optional;

public interface RequestHandler {
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception;

    public default int optionalInt(Request request, String name, int defaultValue) throws UsageException {
        String value = request.getParameter(name);

        try {
            return Integer.parseInt(value);
        } catch (NumberFormatException e) {
            return defaultValue;
        }
    }

    public default String optionalString(Request request, String name, String defaultValue) throws UsageException {
        String value = request.getParameter(name);

        if (value == null) {
            return defaultValue;
        }

        return value;
    }

    public default <T extends Enum<T>> T requiredEnum(Request request, String name, Class<T> klass) throws UsageException {
        String valueString = requiredString(request, name);
        try {
            return Enum.valueOf(klass, valueString);
        } catch (Exception e) {
            throw new UsageException("Invalid value for " + name);
        }
    }
    public default String requiredString(Request request, String name) throws UsageException {
        String value = request.getParameter(name);
        if (value == null) {
            throw new UsageException(String.format("Parameter '%s' is missing", name));
        }

        return value;
    }

    public default String requiredString(Map<String, String> routeAttributes, String name) throws UsageException {
        String value = routeAttributes.get(name);
        if ((value == null)) {
            throw new UsageException(String.format("Parameter '%s' is missing", name));
        }

        return value;
    }

    public default int requiredInt(Map<String, String> routeAttributes, String name) throws UsageException {
        String s = requiredString(routeAttributes, name);
        try {
            return Integer.parseInt(s);
        } catch (Exception e) {
            throw new UsageException(String.format( "Parameter '%s' should be an integer", name ));
        }
    }

    public default long requiredLong(Map<String, String> routeAttributes, String name) throws UsageException {
        String s = requiredString(routeAttributes, name);
        try {
            return Long.parseLong(s);
        } catch (Exception e) {
            throw new UsageException(String.format( "Parameter '%s' should be an integer", name ));
        }
    }

    public default Optional<String> optionalString(Map<String, String> routeAttributes, String name) {
        String value = routeAttributes.get(name);
        return value == null ? Optional.empty() : Optional.of(value);
    }

    default String readBody( final InputStream inputStream )
        throws UsageException
    {
        final StringBuilder body = new StringBuilder( );
        try( final BufferedReader reader = new BufferedReader( new InputStreamReader( inputStream ) ) )
        {
            String line;
            while( ( line = reader.readLine( ) ) != null )
            {
                body.append( line );
            }
        }
        catch ( IOException e )
        {
            throw new UsageException( "Error reading request body.", e );
        }

        return body.toString();
    }
}
