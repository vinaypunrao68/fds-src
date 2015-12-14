/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.rest.v08.model;

import com.formationds.client.ical.RecurrenceRule;
import com.formationds.client.ical.WeekDays;
import com.formationds.client.ical.iCalWeekDays;
import com.formationds.client.v08.model.*;
import com.formationds.client.v08.model.SnapshotPolicy.SnapshotPolicyType;
import com.formationds.client.v08.model.iscsi.Credentials;
import com.formationds.client.v08.model.iscsi.LUN;
import com.formationds.client.v08.model.iscsi.Target;
import com.formationds.client.v08.model.nfs.ACL;
import com.formationds.client.v08.model.nfs.AllSquash;
import com.formationds.client.v08.model.nfs.NfsOptionBase;
import com.formationds.client.v08.model.nfs.RootSquash;
import com.formationds.client.v08.model.nfs.Sync;
import com.google.gson.*;
import com.google.gson.reflect.TypeToken;
import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;

import java.lang.reflect.Type;
import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

public class GSONTest {

    static Gson   gson;
    static Volume testVol1;

    public static class NfsOptionAdapter
            implements JsonSerializer<NfsOptionBase>, JsonDeserializer<NfsOptionBase>
    {
        private static final String CLASSNAME = "CLASSNAME";
        private static final String INSTANCE  = "INSTANCE";

        /**
         * {@inheritDoc}
         */
        @Override
        public NfsOptionBase deserialize( JsonElement json,
                                          Type typeOfT,
                                          JsonDeserializationContext context )
                throws JsonParseException
        {
            JsonObject jsonObject =  json.getAsJsonObject();
            JsonPrimitive prim = ( JsonPrimitive ) jsonObject.get( CLASSNAME ) ;
            String className = prim.getAsString();

            Class<?> klass;
            try
            {
                klass = Class.forName( className) ;
            }
            catch (ClassNotFoundException e)
            {
                throw new JsonParseException( e.getMessage() );
            }

            return context.deserialize( jsonObject.get( INSTANCE ), klass );
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public JsonElement serialize( NfsOptionBase src,
                                      Type typeOfSrc,
                                      JsonSerializationContext context )
        {
            JsonObject retValue = new JsonObject();
            String className = src.getClass().getCanonicalName();
            retValue.addProperty( CLASSNAME, className );
            JsonElement elem = context.serialize( src );
            retValue.add( INSTANCE, elem );
            return retValue;
        }
    }

    public static class VolumeSettingsAdapter implements JsonSerializer<VolumeSettings>, JsonDeserializer<VolumeSettings> {

        private static final String CLASSNAME = "CLASSNAME";
        private static final String INSTANCE  = "INSTANCE";

        @Override
        public JsonElement serialize(VolumeSettings src, Type typeOfSrc,
                                     JsonSerializationContext context) {

            JsonObject retValue = new JsonObject();
            String className = src.getClass().getCanonicalName();
            retValue.addProperty(CLASSNAME, className);
            JsonElement elem = context.serialize(src);
            retValue.add(INSTANCE, elem);
            return retValue;
        }

        @Override
        public VolumeSettings deserialize(JsonElement json, Type typeOfT,
                                   JsonDeserializationContext context) throws JsonParseException  {
            JsonObject jsonObject =  json.getAsJsonObject();
            JsonPrimitive prim = (JsonPrimitive) jsonObject.get(CLASSNAME);
            String className = prim.getAsString();

            Class<?> klass = null;
            try {
                klass = Class.forName(className);
            } catch (ClassNotFoundException e) {
                e.printStackTrace();
                throw new JsonParseException(e.getMessage());
            }
            return context.deserialize(jsonObject.get(INSTANCE), klass);
        }
    }

    static public Gson gson() { return new GsonBuilder().setDateFormat( "YYYYMMdd-hhmmss.SSSSS" )
                                                        .setFieldNamingPolicy( FieldNamingPolicy.IDENTITY )
                                                        .setPrettyPrinting()
                                                        .registerTypeAdapter( VolumeSettings.class,
                                                                              new VolumeSettingsAdapter() )
                                                        .registerTypeAdapter( NfsOptionBase.class,
                                                                              new NfsOptionAdapter( ) )
                                                        .setLongSerializationPolicy(
                                                                LongSerializationPolicy.STRING ).create(); }

    @BeforeClass
    public static void setUpCLass() {
        gson = gson();

        Tenant tenant = new Tenant( 3L, "Dave" );
        VolumeStatus status = new VolumeStatus( VolumeState.Active,
                                                Size.of( 3, SizeUnit.GB ) );

        VolumeSettings settings = new VolumeSettingsBlock( Size.of( 5L, SizeUnit.TB ),
                                                           Size.of( 1, SizeUnit.KB ) );

        RecurrenceRule rule = new RecurrenceRule();
        rule.setFrequency( "WEEKLY" );

        WeekDays days = new WeekDays();
        days.add( iCalWeekDays.MO );

        rule.setDays( days );

        SnapshotPolicy sPolicy = new SnapshotPolicy( SnapshotPolicyType.SYSTEM_TIMELINE, rule, Duration.ofDays( 30 ) );
        List<SnapshotPolicy> sPolicies = new ArrayList<>();
        sPolicies.add( sPolicy );

        QosPolicy qPolicy = new QosPolicy( 3, 0, 2000 );

        DataProtectionPolicy dPolicy = new DataProtectionPolicy( Duration.ofDays( 1L ), sPolicies );

        testVol1 = new Volume( 1L,
                               "TestVolume",
                               tenant,
                               "MarioBrothers",
                               status,
                               settings,
                               MediaPolicy.HDD,
                               dPolicy,
                               VolumeAccessPolicy.exclusiveRWPolicy(),
                               qPolicy,
                               Instant.now(),
                               null );
    }

    public static class ImmutableBean {

        public static ImmutableBean im( Long id, String name ) { return new ImmutableBean( id, name ); }

        private final Long   id;
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

    @Test
    public void testVolumeSimple() {
        Volume v = new Volume( 0L, "test1" );

        String j = gson.toJson( v );
        Assert.assertNotNull( j );

        Volume v2 = gson.fromJson( j, Volume.class );
        Assert.assertNotNull( v2 );
        Assert.assertEquals( v, v2 );

        Assert.assertEquals( v.getId(), v2.getId() );
        Assert.assertEquals( v.getName(), v2.getName() );
    }

    @Test
    public void testSize() {
        Size s1 = Size.of( 1L, SizeUnit.KB );
        String j = gson.toJson( s1 );

        TypeToken.get( s1.getClass() );
        Size s2 = gson.fromJson( j, Size.class );

        Assert.assertEquals( s1, s2 );
    }

    @Test
    public void testVolumeSetting() {
        VolumeSettings v = new VolumeSettingsBlock( Size.of( 1, SizeUnit.YB ), Size.of( 1, SizeUnit.KB ) );

        String j = gson.toJson( v );
        Assert.assertNotNull( j );

        VolumeSettings v2 = gson.fromJson( j, VolumeSettingsBlock.class );
        Assert.assertNotNull( v2 );
        Assert.assertEquals( v, v2 );

        v = new VolumeSettingsObject( Size.of( 1, SizeUnit.YB ) );

        j = gson.toJson( v );
        Assert.assertNotNull( j );

        v2 = gson.fromJson( j, VolumeSettingsObject.class );
        Assert.assertNotNull( v2 );
        Assert.assertEquals( v, v2 );
    }

    @Test
    public void testVolumeSettingNfs() {
        VolumeSettings v = new VolumeSettingsNfs.Builder()
                                                .withOption( new ACL( true ) )
                                                .withOption( new Sync( false ) )
                                                .withOption( new RootSquash( false ) )
                                                .withOption( new AllSquash( false ) )
                                                .withIpFilter( new IPFilter( "10.2.10.*",
                                                                             IPFilter.IP_FILTER_MODE.ALLOW ) )
                                                .withIpFilter( new IPFilter( "*.local.domain",
                                                                             IPFilter.IP_FILTER_MODE.DENY ) )
                                                .build( );
        String j = gson.toJson( v );
        System.out.println( j );
        Assert.assertNotNull( j );

        VolumeSettings v2 = gson.fromJson( j, VolumeSettingsNfs.class );
        Assert.assertNotNull( v2 );
        Assert.assertEquals( v, v2 );
    }

    @Test
    public void testVolumeSettingISCSI() {
        final Target target = new Target.Builder( )
                                        .withLun(
                                            new LUN.Builder( ).withLun( "volume_1" )
                                                              .withAccessType( LUN.AccessType.RW )
                                                              .build() )
                                        .withIncomingUser( new Credentials( "brian",
                                                                             "secret" ) )
                                        .withIncomingUser( new Credentials( "eric",
                                                                             "disco" ) )
                                        .withOutgoingUser( new Credentials( "fds", "fds_secret" ) )
                                        .build( );
        target.setId( 1L );
        target.setName( "Target #" + target.getId() );
        VolumeSettings v =
                new VolumeSettingsISCSI.Builder()
                        .withTarget( target )
                        .build( );
        String j = gson.toJson( v );
        Assert.assertNotNull( j );

        VolumeSettings v2 = gson.fromJson( j, VolumeSettingsISCSI.class );
        Assert.assertNotNull( v2 );
        Assert.assertEquals( v, v2 );
    }

    @Test
    public void testVolume1() {

        String jsonString = gson.toJson( testVol1 );
        Volume readVolume = gson.fromJson( jsonString, Volume.class );

        Assert.assertNotNull( readVolume );
        Assert.assertEquals( testVol1, readVolume );
    }
}
