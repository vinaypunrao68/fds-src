/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.helper;

import com.formationds.client.v08.model.VolumeSettings;
import com.formationds.client.v08.model.VolumeSettingsBlock;
import com.formationds.client.v08.model.VolumeSettingsISCSI;
import com.formationds.client.v08.model.VolumeSettingsNfs;
import com.formationds.client.v08.model.VolumeSettingsObject;
import com.formationds.commons.model.type.Protocol;
import com.google.gson.*;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.LogManager;

import java.io.Reader;
import java.lang.reflect.Field;
import java.lang.reflect.Type;
import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;

/**
 * @author ptinius
 */
public class ObjectModelHelper {

    private static final Logger logger =
        LogManager.getLogger( ObjectModelHelper.class );

    private static final ModelObjectExclusionStrategy exclusion =
        new ModelObjectExclusionStrategy();

    private static final DateFormat ISO_DATE =
        new SimpleDateFormat( "yyyyMMdd'T'HHmmss" );

    /**
     * Implements a GSON Serializer/Deserializer for the v08 VolumeSettings model
     */
    public static class VolumeSettingsAdapter implements JsonDeserializer<VolumeSettings>, JsonSerializer<VolumeSettings> {


        @Override
        public JsonElement serialize( VolumeSettings src, Type typeOfSrc,
                                      JsonSerializationContext context ) {
            return context.serialize( src );
        }

        @Override
        public VolumeSettings deserialize( JsonElement json, Type typeOfT,
                                           JsonDeserializationContext context ) throws JsonParseException {
            JsonObject jsonObject = json.getAsJsonObject();

            Class<?> klass;
            String type = jsonObject.get( "type" ).getAsString();
            
            if ( type.equalsIgnoreCase( "BLOCK" ) )
            {
            	klass = VolumeSettingsBlock.class;
            }
            else if ( type.equalsIgnoreCase( "ISCSI" ) )
            {
                klass = VolumeSettingsISCSI.class;
            }
            else if ( type.equalsIgnoreCase( "NFS" ) )
            {
                klass = VolumeSettingsNfs.class;
            }
            else // must be a object volume
            {
                klass = VolumeSettingsObject.class;
            }

            return context.deserialize( jsonObject, klass );
        }
    }

    /**
     * @param protocols the {@link List} of {@link com.formationds.commons.model.type.Protocol}
     *
     * @return Returns {@link String} representing a comma separated list of
     * {@link com.formationds.commons.model.type.Protocol}
     */
    public static String toProtocalString( final List<Protocol> protocols ) {
        final StringBuilder sb = new StringBuilder();
        for ( final Protocol protocol : protocols ) {
            if ( sb.length() != 0 ) {
                sb.append( "," );
            }

            sb.append( protocol.name() );
        }

        return sb.toString();
    }

    /**
     * @param date the {@link String} representing the RDATE spec format
     *
     * @return Returns {@link Date}
     *
     * @throws ParseException if there is a parse error
     */
    public static Date toiCalFormat( final String date )
        throws ParseException {
        return fromISODateTime( date );
    }

    /**
     * @param date the {@link Date} representing the date
     *
     * @return Returns {@link String} representing the ISO Date format of
     * "yyyyMMddTHHmmssZ"
     */
    public static String isoDateTimeUTC( final Date date ) {
        return ISO_DATE.format( date );
    }

    /**
     * @param date the {@link String} representing the ISO Date format of
     *             "yyyyMMddThhmmss"
     *
     * @return Date Returns {@link Date} representing the {@code date}
     */
    public static Date fromISODateTime( final String date )
        throws ParseException {
        return ISO_DATE.parse( date );
    }

    /**
     * @param field the {@link java.lang.reflect.Field} representing the field
     *              to be excluded.
     */
    public static void excludeField( final Field field ) {
        try {
            exclusion.excludeField( field );
        } catch ( ClassNotFoundException e ) {
            logger.warn( "could not find either the class {} or the field {}, skipping!",
                         field.getDeclaringClass(),
                         field.getName() );
        }
    }

    /**
     * @param clazz the {@link Class} representing the class to exclude
     */
    public static void excludeClass( final Class<?> clazz ) {
        exclusion.excludeClass( clazz );
    }

    /**
     * @param json the {@link String} representing the JSON object
     * @param type the {@link Type} to parse the JSON into
     *
     * @return Returns the {@link Type} represented within the JSON
     */
    public static <T> T toObject( final String json, final Type type ) {
        return gson().fromJson( json, type );
    }

    /**
     * @param reader the {@link java.io.Reader}
     * @param type   the {@link Type} to parse the JSON into
     *
     * @return Returns the {@link Type} represented within the JSON
     */
    public static <T> T toObject( final Reader reader, final Type type ) {
        return gson().fromJson( reader, type );
    }

    /**
     * @param object the {@link Object} representing the JSON
     *
     * @return Returns the {@link String} representing the JSON
     */
    public static String toJSON( final Object object ) {
        return gson().toJson( object );
    }

    /**
     * @return a Gson instance configured with the default serialization settings
     */
    public static Gson gson() {
        return new GsonBuilder().setDateFormat( "YYYYMMdd-hhmmss.SSSSS" )
                                .setFieldNamingPolicy( FieldNamingPolicy.IDENTITY )
                                .setLongSerializationPolicy( LongSerializationPolicy.STRING )
                                .registerTypeAdapter( VolumeSettings.class, new VolumeSettingsAdapter() )
                                .setPrettyPrinting()
                                .create();
    }

    /**
     * default singleton
     */
    private ObjectModelHelper() {
        super();
    }
}
