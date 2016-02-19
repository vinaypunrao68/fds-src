/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.iscsi;

import java.util.Objects;

/**
 * @author ptinius
 */
public class ISNSServer
{
    public static class Builder {
        private String host;
        private int port;

        public String getHost( )
        {
            return host;
        }

        public int getPort( )
        {
            return port;
        }

        public Builder withPort( final int port )
        {
            this.port = port;
            return this;
        }

        public Builder withHost( final String host )
        {
            this.host = host;
            return this;
        }

        public ISNSServer build( ) {
            return new ISNSServer( getHost(), getPort() );
        }
    }

    private final String host;
    private final int port;

    protected ISNSServer( final String host, final int port )
    {
        this.host = host;
        this.port = port;
    }

    /**
     * @return Returns the internet storage service server name
     */
    public String getHost( ) { return host; }

    /**
     * @return Returns the internet storage service server port number
     */
    public int getPort( ) { return port; }

    @Override
    public boolean equals( final Object o )
    {
        if ( this == o ) return true;
        if ( !( o instanceof ISNSServer ) ) return false;
        final ISNSServer that = ( ISNSServer ) o;
        return getPort( ) == that.getPort( ) &&
            Objects.equals( getHost( ), that.getHost( ) );
    }

    @Override
    public int hashCode( )
    {
        return Objects.hash( getHost( ), getPort( ) );
    }

    @Override
    public String toString( )
    {
        return getHost() + ":" + getPort();
    }
}
