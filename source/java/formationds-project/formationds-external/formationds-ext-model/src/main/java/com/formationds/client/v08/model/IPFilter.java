/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model;

import java.util.Optional;

/**
 * @author ptinius
 */
public class IPFilter
{
    public static class Builder
    {
        private String starting = null;
        private String ending = null;
        private IP_FILTER_MODE mode;

        public String getStarting( )
        {
            return starting;
        }

        public Builder withStarting( final String starting )
        {
            this.starting = starting;
            return this;
        }

        public String getEnding( )
        {
            return ending;
        }

        public Builder withEnding( final String ending )
        {
            this.ending = ending;
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
            return new IPFilter( getStarting(), getEnding(), getMode() );
        }
    }

    public enum IP_FILTER_MODE
    {
        DENY,
        ALLOW
    }

    private Optional<String> starting = Optional.empty();
    private Optional<String> ending = Optional.empty();
    private IP_FILTER_MODE mode;

    public IPFilter( )
    {
        this( null, null, IP_FILTER_MODE.ALLOW );
    }

    public IPFilter( final String starting, final IP_FILTER_MODE mode )
    {
        this( starting, null, mode );
    }

    public IPFilter( final String starting, final String ending, final IP_FILTER_MODE mode )
    {
        this.starting = Optional.ofNullable( starting );
        this.ending = Optional.ofNullable( ending );
        this.mode = mode;
    }

    /**
     * @return Returns the starting IP, if no ending IP is provided this is treated as a single IP to be {@link #getMode()}
     */
    public Optional< String > getStarting( )
    {
        return starting;
    }

    /**
     * @param starting the starting IP, if no ending IP is provided this is treated as a single IP to be {@link #getMode()}
     */
    public void setStarting( final String starting )
    {
        this.starting = Optional.ofNullable( starting );
    }

    /**
     * @return Returns the ending IP, which defined the end of the filter range
     */
    public Optional< String > getEnding( )
    {
        return ending;
    }

    /**
     * @param ending the ending IP, which defined the end of the filter range
     */
    public void setEnding( final String ending )
    {
        this.ending = Optional.ofNullable( ending );
    }

    /**
     * @return Returns the IP filter mode as defined {@link IP_FILTER_MODE}
     */
    public IP_FILTER_MODE getMode( )
    {
        return mode;
    }

    /**
     * @param mode the IP filter mode as defined {@link IP_FILTER_MODE}
     */
    public void setMode( IP_FILTER_MODE mode )
    {
        this.mode = mode;
    }
}
