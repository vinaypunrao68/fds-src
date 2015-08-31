/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.tools.statgen;

import com.formationds.client.v08.model.VolumeSettings;
import com.formationds.client.v08.model.VolumeSettingsBlock;
import com.formationds.client.v08.model.VolumeSettingsObject;
import com.google.gson.*;
import com.sun.jersey.api.client.Client;
import com.sun.jersey.api.client.ClientResponse;
import com.sun.jersey.api.client.WebResource;
import com.sun.jersey.api.client.config.DefaultClientConfig;
import com.sun.jersey.api.uri.UriBuilderImpl;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import java.io.Reader;
import java.lang.reflect.Type;
import java.net.URI;

public class RestClient {
    protected static final Logger logger =
        LogManager.getLogger( RestClient.class );

    public static class ClientException extends RuntimeException {
        public ClientException( String message ) {
            super( message );
        }

        public ClientException( String message, Throwable cause ) {
            super( message, cause );
        }

        public ClientException( Throwable cause ) {
            super( cause );
        }
    }

    protected static class GSON {

        /**
         * Implements a GSON Serializer/Deserializer for the v08 VolumeSettings model
         */
        public static class VolumeSettingsAdapter implements JsonDeserializer<VolumeSettings>,
                                                             JsonSerializer<VolumeSettings> {

            @Override
            public JsonElement serialize( VolumeSettings src, Type typeOfSrc,
                                          JsonSerializationContext context ) {

                JsonElement elem = context.serialize( src );
                return elem;
            }

            @Override
            public VolumeSettings deserialize( JsonElement json, Type typeOfT,
                                               JsonDeserializationContext context ) throws JsonParseException {
                JsonObject jsonObject = json.getAsJsonObject();

                Class<?> klass = null;
                String type = jsonObject.get( "type" ).getAsString();

                if ( type.equalsIgnoreCase( "BLOCK" ) ) {
                    klass = VolumeSettingsBlock.class;
                } else {
                    klass = VolumeSettingsObject.class;
                }

                return context.deserialize( jsonObject, klass );
            }
        }

        private static final Gson GSON = new GsonBuilder().setDateFormat( "YYYYMMdd-hhmmss.SSSSS" )
                                                          .setFieldNamingPolicy( FieldNamingPolicy.IDENTITY )
                                                          .setLongSerializationPolicy( LongSerializationPolicy.STRING )
                                                          .registerTypeAdapter( VolumeSettings.class,
                                                                                new GSON.VolumeSettingsAdapter() )
                                                          .setPrettyPrinting()
                                                          .create();

        /**
         * @param reader the {@link java.io.Reader}
         * @param type   the {@link Type} to parse the JSON into
         *
         * @return Returns the {@link Type} represented within the JSON
         */
        public static <T> T toObject( final Reader reader, final Type type ) {
            return GSON.fromJson( reader, type );
        }

        /**
         * @param json the {@link String} representing the JSON object
         * @param type the {@link Type} to parse the JSON into
         *
         * @return Returns the {@link Type} represented within the JSON
         */
        public static <T> T toObject( final String json, final Type type ) {
            return GSON.fromJson( json, type );
        }

        /**
         * @param object the {@link Object} representing the JSON
         *
         * @return Returns the {@link String} representing the JSON
         */
        public static String toJSON( final Object object ) {
            return GSON.toJson( object );
        }
    }

    protected final String  protocol;
    protected final String  host;
    protected final Integer httpPort;
    private final   String  urlPrefix;

    private Client      client      = null;
    private WebResource webResource = null;

    public RestClient( final String host,
                       final Integer port,
                       final String protocol,
                       final String urlPrefix ) {
        super();
        this.host = host;
        this.httpPort = port;
        this.protocol = protocol;
        this.urlPrefix = urlPrefix;
    }

    public String getProtocol() {
        return protocol;
    }

    public String getHost() {
        return host;
    }

    public Integer getHttpPort() {
        return httpPort;
    }

    public String getUrlPrefix() {
        return urlPrefix;
    }

    private Client client() {

        if ( client == null ) {

            client = Client.create( new DefaultClientConfig() );
        }

        return client;
    }

    public WebResource webResource() {

        if ( webResource == null ) {

            final URI uri =
                new UriBuilderImpl().scheme( protocol )
                                    .host( host )
                                    .port( httpPort )
                                    .path( urlPrefix )
                                    .build();

            webResource = client().resource( uri );
        }

        return webResource;
    }

    public void isOk( final ClientResponse response )
        throws ClientException {
        if ( !response.getClientResponseStatus()
                      .equals( ClientResponse.Status.OK ) ) {

            logger.error( "ISOK::RESPONSE::" + response.toString() );

            throw new ClientException( response.toString() );
        } else {

            logger.debug( "ISOK::RESPONSE::" + response.toString() );
        }
    }
}
