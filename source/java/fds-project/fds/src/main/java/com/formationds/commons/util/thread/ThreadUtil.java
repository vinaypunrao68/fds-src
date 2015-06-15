/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util.thread;

import com.formationds.commons.util.i18n.CommonsUtilResource;

import java.io.PrintWriter;
import java.lang.Thread.UncaughtExceptionHandler;
import java.lang.management.ManagementFactory;
import java.lang.management.ThreadInfo;
import java.lang.management.ThreadMXBean;
import java.text.MessageFormat;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @author ptinius
 */
@SuppressWarnings( { "unused" } )
public class ThreadUtil
{
  static private ThreadMXBean threadBean = ManagementFactory.getThreadMXBean();

  /**
   * @param prefix
   * @return a new ThreadFactory that creates new daemon threads with the specified prefix and sequential counter.
   */
  public static ThreadFactory newThreadFactory( String prefix ) {
    return newThreadFactory( null,
                             prefix,
                             true,
                             Thread.NORM_PRIORITY,
                             Thread.getDefaultUncaughtExceptionHandler() );
  }

  /**
   * @param prefix
   * @param daemon
   * @return a new thread factory that creates new threads with the specified prefix
   */
  public static ThreadFactory newThreadFactory( String prefix,
                                                boolean daemon ) {
      return newThreadFactory( null,
                               prefix,
                               daemon,
                               Thread.NORM_PRIORITY,
                               Thread.getDefaultUncaughtExceptionHandler() );
  }

  /**
   *
   * @param group
   * @param prefix
   * @param daemon
   * @param priority
   * @param uncaughtExceptionHandler
   * @return a new thread factory that creates new threads with the specified prefix
   */
  public static ThreadFactory newThreadFactory( ThreadGroup group,
                                                String prefix,
                                                boolean daemon,
                                                int priority,
                                                UncaughtExceptionHandler uncaughtExceptionHandler ) {
      final AtomicInteger seq = new AtomicInteger( 1 );
      return (r) -> {
          String threadName = String.format( "%s-%d", prefix, seq.getAndIncrement() );
          final Thread t = new Thread( group, r, threadName );
          t.setDaemon( daemon );
          if ( t.getPriority() != priority ) {
              t.setPriority( priority );
          }
          if (uncaughtExceptionHandler != null ) {
              t.setUncaughtExceptionHandler( uncaughtExceptionHandler );
          }
          return t;
      };
  }

  /**
   * @param val the boolean flag
   */
  public static void setContentionTracing(boolean val)
  {
    threadBean.setThreadContentionMonitoringEnabled( val );
  }

  /**
   * @return Returns the current thread name
   */
  public static String getThreadName()
  {
    return Thread.currentThread().getName();
  }

  /**
   * @param id the id
   * @param name the name
   *
   * @return Returns the task name
   */
  private static String getTaskName(long id, String name)
  {
    if( name == null )
    {
      return Long.toString( id );
    }
    return id + " ( " + name + " )";
  }

  /**
   * Print all of the thread's information and stack traces.
   *
   * @param stream the stream to
   * @param title  a string title for the stack trace
   */
  public static void printThreadInfo(PrintWriter stream, String title)
  {
    final int STACK_DEPTH = 20;
    boolean contention = threadBean.isThreadContentionMonitoringEnabled();
    long[] threadIds = threadBean.getAllThreadIds();
    stream.println( "Process Thread Dump: " + title );
    stream.println( threadIds.length + " active threads" );
    for( long tid : threadIds )
    {
      ThreadInfo info = threadBean.getThreadInfo( tid, STACK_DEPTH );
      if( info == null )
      {
        stream.println( "  Inactive" );
        continue;
      }
      stream.println( "Thread " +
                      getTaskName( info.getThreadId(),
                                   info.getThreadName() ) + ":" );

      Thread.State state = info.getThreadState();

      stream.println( "  State: " + state );
      stream.println( "  Blocked count: " + info.getBlockedCount() );
      stream.println( "  Waited count: " + info.getWaitedCount() );

      if( contention )
      {
        stream.println( "  Blocked time: " + info.getBlockedTime() );
        stream.println( "  Waited time: " + info.getWaitedTime() );
      }

      if( state == Thread.State.WAITING )
      {
        stream.println( "  Waiting on " + info.getLockName() );
      }
      else if( state == Thread.State.BLOCKED )
      {
        stream.println( "  Blocked on " + info.getLockName() );
        stream.println( "  Blocked by "
                        + getTaskName( info.getLockOwnerId(),
                                       info.getLockOwnerName() ) );
      }

      stream.println( "  Stack:" );
      for( StackTraceElement frame : info.getStackTrace() )
      {
        stream.println( "    " + frame.toString() );
      }
    }

    stream.flush();
  }

  /**
   * @param t the {@link Thread}
   * @param indent the indent prefix
   */
  public static void printThreadInfo(Thread t, String indent)
  {
    if( t == null )
    {
      return;
    }

    System.out.println( indent + "Thread: " + t.getName() + "  Priority: "//NOPMD
                        + t.getPriority() + ( t.isDaemon()
                                              ? " Daemon"
                                              : "" )
                        + ( t.isAlive()
                            ? ""
                            : " Not Alive" ) );
  }

  /**
   * Display info about a thread group
   *
   * @param g the {@link ThreadGroup}
   * @param indent the indent prefix
   */
  public static void printGroupInfo(ThreadGroup g, String indent)
  {
    if( g == null )
    {
      return;
    }
    int numThreads = g.activeCount();
    int numGroups = g.activeGroupCount();
    Thread[] threads = new Thread[ numThreads ];
    ThreadGroup[] groups = new ThreadGroup[ numGroups ];//NOPMD

    g.enumerate( threads, false );
    g.enumerate( groups, false );

    System.out.println( indent + "Thread Group: " + g.getName()//NOPMD
                        + "  Max Priority: " + g.getMaxPriority()
                        + ( g.isDaemon()
                            ? " Daemon"
                            : "" ) );

    for( int i = 0; i < numThreads; i++ )
    {
      printThreadInfo( threads[ i ], indent + "    " );
    }
    for( int i = 0; i < numGroups; i++ )
    {
      printGroupInfo( groups[ i ], indent + "    " );
    }
  }

  /**
   * Find the root thread group and list it recursively
   */
  public static void listAllThreads()
  {
    ThreadGroup currentThreadGroup;
    ThreadGroup rootThreadGroup;
    ThreadGroup parent;

    // Get the current thread group
    currentThreadGroup = Thread.currentThread().getThreadGroup();//NOPMD

    // Now go find the root thread group
    rootThreadGroup = currentThreadGroup;
    parent = rootThreadGroup.getParent();
    while( parent != null )
    {
      rootThreadGroup = parent;
      parent = parent.getParent();
    }

    printGroupInfo( rootThreadGroup, "" );
  }


  private static long previousLogTime = 0;

  /**
   * Return the correctly-typed {@link Class} of the given object.
   *
   * @param o object whose correctly-typed <code>Class</code> is to be
   *          obtained
   *
   * @return the correctly typed <code>Class</code> of the given object.
   */
  @SuppressWarnings( "unchecked" )
  public static <T> Class<T> getClass(T o)
  {
    return ( Class<T> ) o.getClass();
  }

  /**
   * @param millis the milliseconds to sleep
   */
  public static void sleep(final long millis)
  {
    try
    {
      Thread.sleep( millis );
    }
    catch( InterruptedException ignore )
    {
    }
  }

  private static final String STARTUP_MSG =
    CommonsUtilResource.getString( "startup.thread.messages" );
  private static final String SHUTDOWN_MSG =
    CommonsUtilResource.getString( "shutdown.thread.messages" );

  /**
   * @param threadName the new name for this thread.
   *
   * @return Returns the thread startup message
   */
  public static String getThreadStartupMessage(final String threadName)
  {
    return MessageFormat.format( STARTUP_MSG, threadName );
  }

  /**
   * @param threadName the new name for this thread.
   *
   * @return Returns the thread shutdown message
   */
  public static String getThreadShutdownMessage(final String threadName)
  {
    return MessageFormat.format( SHUTDOWN_MSG, threadName );
  }

  /**
   *
   */
  private ThreadUtil()
  {
    super();
  }
}
