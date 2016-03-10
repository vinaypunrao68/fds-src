/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.redis;

import java.util.concurrent.Semaphore;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * @author ptinius
 */
public enum RedisSingleton
{
    INSTANCE;

    private Redis api = null;

    private int totalPermits = Integer.MAX_VALUE;
    private Semaphore redisLock = new Semaphore(totalPermits);

    private static final Logger logger = LoggerFactory.getLogger( RedisSingleton.class );

    public Redis api()
    {
        if( api == null )
        {
            api = new Redis( );
        }

        return api;
    }

    public void getRedisLock() throws InterruptedException
    {
    	logger.trace("Acquiring redis lock");
    	redisLock.acquire();
    }
    
    public void releaseRedisLock()
    {
    	logger.trace("Releasing redis lock");
    	redisLock.release();
    }
    
    public void waitRedis() throws InterruptedException
    {
    	logger.trace("Waiting for redis calls to finish");
    	redisLock.acquire(totalPermits);
    }
}
