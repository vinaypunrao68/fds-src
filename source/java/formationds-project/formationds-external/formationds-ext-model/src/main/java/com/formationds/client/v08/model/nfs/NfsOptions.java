/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.nfs;

/**
 * @author ptinius
 */
public class NfsOptions
{
    public static class Builder
    {
        public Builder ro() { this.ro = true; return this; }
        public Builder rw() { this.rw = true; return this; }
        public Builder withAcl() { _withAcl = true; return this; }
        public Builder withoutAcl() { _withAcl = false; return this; }
        public Builder async() { async = true; return this; }
        public Builder allSquash() { allSquash = true; noAllSquash = false; return this; }
        public Builder noAllSquash() { noAllSquash = true; allSquash = false; return this; }
        public Builder withRootSquash() { rootSquash = true; return this; }

        public NfsOptions build( )
        {
            return new NfsOptions( this );
        }

        private boolean ro = false;
        private boolean rw = false;
        private boolean _withAcl = false;
        private boolean allSquash = false;
        private boolean noAllSquash = false;
        private boolean rootSquash = false;
        private boolean async = false;
    }

    private String options;

    NfsOptions( final Builder builder )
    {
        final StringBuilder sb = new StringBuilder( );

        if( builder.ro && !builder.rw )
        {
            sb.append( "ro" ).append( "," );
        }
        else if( !builder.ro && builder.rw )
        {
            sb.append( "rw" ).append( "," );
        }

        sb.append( builder.async ? "async" : "sync" )
          .append( "," )
          .append( builder._withAcl ? "acl" : "noacl" )
          .append( "," )
          .append( builder.rootSquash ? "root_squash" : "no_root_squash" );

        if( builder.allSquash )
        {
            sb.append( "," )
              .append( "all_squash" );
        }
        else if( builder.noAllSquash )
        {
            sb.append( "," )
              .append( "no_all_squash" );
        }

        this.options = sb.toString();
    }

    public String getOptions( ) { return this.options; }

    @Override
    public String toString( )
    {
        return getOptions();
    }
}
