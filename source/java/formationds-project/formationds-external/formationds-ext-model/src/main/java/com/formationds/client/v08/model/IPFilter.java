/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model;

import java.util.Objects;
import java.util.Optional;

/**
 * @author ptinius
 */
public class IPFilter
{
    public static class Builder
    {
        public String getPattern( )
        {
            return pattern;
        }

        public Builder withPattern( final String pattern )
        {
            this.pattern = pattern;
            return this;
        }

        public IP_FILTER_MODE getMode( )
        {
            return mode;
        }

        public Builder withMode( IP_FILTER_MODE mode )
        {
            this.mode = mode;
            return this;
        }

        public IPFilter build()
        {
            return new IPFilter( getPattern(), getMode() );
        }

        private String pattern = null;
        private IP_FILTER_MODE mode;
    }

    public enum IP_FILTER_MODE { DENY, ALLOW }

    private Optional<String> pattern = Optional.empty();
    private IP_FILTER_MODE mode;

    public IPFilter( ) { this( null, IP_FILTER_MODE.ALLOW ); }

    /**
     *
     */
    public IPFilter( final String pattern, final IP_FILTER_MODE mode )
    {
        this.pattern = Optional.ofNullable( pattern );
        this.mode = mode;
    }

    /**
     * @return Returns the filter pattern to be {@link IP_FILTER_MODE#ALLOW} or {@link IP_FILTER_MODE#DENY}
     */
    public Optional<String> getPattern( ) { return pattern; }

    /**
     * @return Returns the IP filter mode as defined {@link IP_FILTER_MODE}
     */
    public IP_FILTER_MODE getMode( ) { return mode; }

    @Override
    public boolean equals( final Object o )
    {
        if ( this == o ) return true;
        if ( !( o instanceof IPFilter ) ) return false;
        final IPFilter ipFilter = ( IPFilter ) o;
        return Objects.equals( getPattern( ), ipFilter.getPattern( ) ) &&
            getMode( ) == ipFilter.getMode( );
    }

    @Override
    public int hashCode( )
    {
        return Objects.hash( getPattern( ), getMode( ) );
    }
}
