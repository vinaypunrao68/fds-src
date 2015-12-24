/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.redis;

import org.apache.commons.lang3.StringUtils;
import redis.clients.jedis.JedisPool;
import redis.clients.jedis.JedisPoolConfig;
import redis.clients.jedis.Protocol;

import java.net.URI;
import java.net.URISyntaxException;

/**
 * @author ptinius
 */
@SuppressWarnings( "WeakerAccess" )
public class RedisPoolFactory
    implements PoolFactory<JedisPool>
{
    private static final String REDIS_URI_FMT_WITH_PWD =
        "redis://:%s@%s:%d/%d";
    private static final String REDIS_URI_FMT =
        "redis://%s:%d/%d";

    private String host = "localhost";
    private int port = Protocol.DEFAULT_PORT;
    private String password = null;
    private int timeout = Protocol.DEFAULT_TIMEOUT;
    private int database = Protocol.DEFAULT_DATABASE;

    @SuppressWarnings( "FieldCanBeLocal" )
    private int maxTotal = 500;

    public String getHost( ) { return host; }
    public String getPassword( ) { return password; }
    public int getPort( ) { return port; }
    public int getTimeout( ) { return timeout; }
    public int getDatabase( ) { return database; }
    public int getMaxTotal( ) { return  maxTotal; }

    public JedisPool build( )
    {
        JedisPoolConfig config = new JedisPoolConfig( );
        config.setMaxTotal( getMaxTotal() );

        try
        {
            if( StringUtils.trimToNull( password ) == null )
            {
                return new JedisPool( config,
                                      new URI(
                                          String.format( REDIS_URI_FMT,
                                                         getHost(),
                                                         getPort(),
                                                         getDatabase() ) ),
                                      getTimeout() );
            }
            else
            {
                return new JedisPool( config,
                                      new URI(
                                          String.format( REDIS_URI_FMT_WITH_PWD,
                                                         StringUtils.trimToNull( getPassword( ) ),
                                                         getHost(),
                                                         getPort(),
                                                         getDatabase() ) ),
                                      getTimeout() );
            }
        }
        catch( URISyntaxException e )
        {
            return new JedisPool( config,
                                  host,
                                  port,
                                  timeout,
                                  StringUtils.trimToNull( password ),
                                  database );
        }
    }
}
