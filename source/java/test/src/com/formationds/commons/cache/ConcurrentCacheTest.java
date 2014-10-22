/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.cache;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @author ptinius
 */
public class ConcurrentCacheTest {
  private static final transient Logger logger =
    LoggerFactory.getLogger( ConcurrentCacheTest.class );

  static final ConcurrentCache<String, String> CACHE =
    new ConcurrentCache<>( 500, 5 * ( 60 * 1000 ) );

  public static void main( String[] args )
    throws InterruptedException {
    new ConcurrentCacheTest().doit();
  }

  ConcurrentCacheTest() {
    super();
  }

  public void doit()
    throws InterruptedException {
    List<Producer> threads = new ArrayList<>();

    for( int t = 0;
         t <= 1000;
         t++ ) {
      final Producer producer = new Producer( CACHE );
      threads.add( producer );
    }

    for( final Producer producer : threads ) {
      producer.start();
    }

    for( final Producer producer : threads ) {
      producer.thread.join();
    }

    Set<String> keys = CACHE.keySet();
    for( final String key : keys ) {
      System.out.println( "key: " + key + " value: " + CACHE.get( key ) );
    }

    System.out.println( "CACHE HITS: " + CACHE.getCacheHits() );
    System.out.println( "CACHE Misses: " + CACHE.getCacheMisses() );
    System.out.println( "CACHE Max Size: " + CACHE.getMaxCacheSize() );
    System.out.println( "CACHE Max Lifetime: " + CACHE.getMaxLifetime() );
  }

  static AtomicInteger sequencer = new AtomicInteger( 0 );

  static class Producer
    implements Runnable {
    private Thread thread;
    private final ConcurrentCache<String, String> cacheInstance;

    Producer( final ConcurrentCache<String, String> cacheInstance ) {
      super();

      this.thread = new Thread( this,
                                "Producer Thread " + sequencer.getAndIncrement() );
      this.cacheInstance = cacheInstance;
    }

    /**
     * starts
     */
    public void start() {
      thread.start();
    }

    /**
     * stops
     */
    public void stop() {
      thread.interrupt();
    }

    /**
     * When an object implementing interface <code>Runnable</code> is used to
     * create a thread, starting the thread causes the object's
     * <code>run</code> method to be called in that separately executing
     * thread.
     * <p>
     * The general contract of the method <code>run</code> is that it may take
     * any action whatsoever.
     *
     * @see Thread#run()
     */
    @Override
    public void run() {
      cacheInstance.put( thread.getName(),
                         getClass().getSimpleName()
                           + "_" + thread.getName() +
                           "_" + cacheInstance.size() );
    }
  }
}
