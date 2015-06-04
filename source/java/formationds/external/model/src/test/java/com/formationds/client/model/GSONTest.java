/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.model;

import com.google.gson.FieldNamingPolicy;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.LongSerializationPolicy;
import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;

import java.util.Objects;

public class GSONTest {

    static Gson gson;

    @BeforeClass
    public static void setUpCLass() {
        gson = new GsonBuilder().setDateFormat( "YYYYMMdd-hhmmss.SSSSS" )
                                       .setFieldNamingPolicy( FieldNamingPolicy.IDENTITY )
                                       .setPrettyPrinting()
                                       .setLongSerializationPolicy( LongSerializationPolicy.STRING ).create();
    }


    public static class ImmutableBean {

        public static ImmutableBean im(Long id, String name) { return new ImmutableBean( id, name ); }

        private final Long id;
        private final String name;

        private ImmutableBean( Long id, String name ) {
            this.id = id;
            this.name = name;
        }

        public Long getId() {
            return id;
        }

        public String getName() {
            return name;
        }

        @Override
        public boolean equals( Object o ) {
            if ( this == o ) { return true; }
            if ( !(o instanceof ImmutableBean) ) { return false; }
            final ImmutableBean that = (ImmutableBean) o;
            return Objects.equals( id, that.id ) &&
                   Objects.equals( name, that.name );
        }

        @Override
        public int hashCode() {
            return Objects.hash( id, name );
        }

        @Override
        public String toString() {
            final StringBuilder sb = new StringBuilder( "ImmutableBean{" );
            sb.append( "id=" ).append( id );
            sb.append( ", name='" ).append( name ).append( '\'' );
            sb.append( '}' );
            return sb.toString();
        }
    }

    @Test
    public void testImmutable() {
        ImmutableBean imb = new ImmutableBean( 1L, "IMB" );
        String j = gson.toJson( imb );
        Assert.assertNotNull( j );

        ImmutableBean imb2 = gson.fromJson( j, ImmutableBean.class );
        Assert.assertNotNull( imb2 );
        Assert.assertEquals( imb, imb2 );
    }

}
