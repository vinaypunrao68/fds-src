/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util.thread;

import com.formationds.commons.util.i18n.CommonsUtilResource;
import org.apache.commons.lang3.time.StopWatch;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.text.MessageFormat;

/**
 * @author $Author: ptinius $
 * @version $Revision: #1 $ $DateTime: 2013/10/24 16:16:16 $
 */
public abstract class ThreadBase
  implements Runnable
{
  protected static final Logger logger =
    LoggerFactory.getLogger( ThreadBase.class );

  protected static final String STARTUP_MSG =
    CommonsUtilResource.getString( "startup.thread.messages" );
  protected static final String SHUTDOWN_MSG =
    CommonsUtilResource.getString( "shutdown.thread.messages" );
  protected static final String SHUTDOWN_MSG_NO_TIMER =
    CommonsUtilResource.getString( "shutdown.thread.messages.no.timer" );

  protected Thread thread = null;
  private final StopWatch timer;

  /**
   * Constructs an abstract thread object
   */
  protected ThreadBase()
  {
    super();

    this.timer = new StopWatch();
  }

  /**
   * Starts the thread
   */
  public void start()
  {
    if( thread == null )
    {
      logger.debug( MessageFormat.format( STARTUP_MSG, threadName() ) );

      thread = new Thread( this );
      thread.setName( threadName() );
      thread.setDaemon( true );
      thread.start();

      if( showTimer() )
      {
        timer.start();
      }
    }
    else
    {
      stop();
      start();
    }
  }

  /**
   * @return Returns the thread name
   */
  public abstract String threadName();

  /**
   * @return Returns <code>true</code> is execution timer should be shown.
   *         Otherwise <code>false</code> is returned.
   */
  public abstract boolean showTimer();

  /**
   * Stops the thread
   */
  public void stop()
  {
    if( thread != null )
    {
      thread.interrupt();

      if( showTimer() )
      {
        timer.stop();
        logger.debug( MessageFormat.format( SHUTDOWN_MSG,
                                            threadName(),
                                            timer ) );
      }
      else
      {
        logger.debug( MessageFormat.format( SHUTDOWN_MSG_NO_TIMER,
                                            threadName() ) );
      }

      thread = null;
    }
  }

  /**
   * @throws SecurityException if the current thread cannot modify this thread
   */
  public void interrupt()
  {
    if( thread != null )
    {
      thread.interrupt();
    }
  }

  /**
   * @throws  InterruptedException if any thread has interrupted the current
   *                               thread. The <i>interrupted status</i> of the
   *                               current thread is cleared when this
   *                               exception is thrown.
   */
  public void join()
    throws InterruptedException
  {
    if( thread != null )
    {
      thread.join();
    }
  }

  /**
   * @param  millis the time to wait in milliseconds
   *
   * @throws IllegalArgumentException if the value of {@code millis} is negative
   *
   * @throws InterruptedException if any thread has interrupted the current
   *                              thread. The <i>interrupted status</i> of
   *                              the current thread is cleared when this
   *                              exception is thrown.
   */
  public final synchronized void join( final long millis )
    throws InterruptedException
  {
    if( thread != null )
    {
      thread.join( millis );
    }
  }

  /**
   * @param  millis the time to wait in milliseconds
   *
   * @param  nanos {@code 0-999999} additional nanoseconds to wait
   *
   * @throws IllegalArgumentException if the value of {@code millis} is negative
   *
   * @throws InterruptedException if any thread has interrupted the current
   *                              thread. The <i>interrupted status</i> of
   *                              the current thread is cleared when this
   *                              exception is thrown.
   */
  public final synchronized void join( final long millis, final int nanos )
    throws InterruptedException
  {
    if( thread != null )
    {
      thread.join( millis, nanos );
    }
  }

  /**
   * @param  on  if {@code true}, marks this thread as a daemon thread
   *
   * @throws  IllegalThreadStateException if this thread is not alive
   *
   * @throws  SecurityException
   *          if its determines that the current thread cannot modify this thread
   */
  public final void setDaemon( final boolean on )
  {
    if( thread != null )
    {
      thread.setDaemon( on );
    }
  }

  /**
   * @return  <code>true</code> if this thread is a daemon thread;
   *          <code>false</code> otherwise.
   * @see     #setDaemon(boolean)
   */
  public boolean isDaemon()
  {
    return thread != null && thread.isDaemon();
  }

  /**
   * Tests if this thread is alive. A thread is alive if it has
   * been started and has not yet died.
   *
   * @return  <code>true</code> if this thread is alive;
   *          <code>false</code> otherwise.
   */
  public final boolean isAlive()
  {
    return thread != null && thread.isAlive();
  }

  /**
   * @return  <code>true</code> if this thread has been interrupted;
   *          <code>false</code> otherwise.
   */
  public boolean isInterrupted()
  {
    return thread != null && thread.isInterrupted();
  }

  /**
   * @param handler the object to use as this thread's uncaught exception
   *                handler. If <tt>null</tt> then this thread has no explicit
   *                handler.
   */
  public void uncaughtExceptionHandler(
    final Thread.UncaughtExceptionHandler handler )
  {
    thread.setUncaughtExceptionHandler( handler );
  }
}
