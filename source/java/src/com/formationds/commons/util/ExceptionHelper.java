/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.function.Consumer;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.TunneledException;
import com.formationds.commons.util.functional.ExceptionThrowingConsumer;
import com.formationds.commons.util.functional.ExceptionThrowingRunnable;

// @eclipseFormat:off
/**
 * @author ptinius
 */
public final class ExceptionHelper {
  private static final Logger logger =
    LoggerFactory.getLogger( ExceptionHelper.class );

  /**
   * @param t the {@link java.lang.Throwable}
   *
   * @return Returns {@link String} representing the exception stack trace
   */
  public static String toString( final Throwable t ) {
    try( final StringWriter str = new StringWriter();
         final PrintWriter writer = new PrintWriter( str ); ) {
      t.printStackTrace( writer );
      return String.valueOf( str.getBuffer() );
    } catch( IOException e ) {
      logger.warn( e.getMessage() );
      logger.trace( "", e );
    }

    return "(null)";
  }
  // @eclipseFormat:on

    public static <T extends Throwable> void rethrowTunneled(TunneledException te,
                                                             Class<T> tunnelType) throws T
    {
        if (te == null) throw new NullArgumentException("te");
        if (tunnelType == null) throw new NullArgumentException("tunnelType");

        try
        {
            throw te.getCause();
        }
        catch (RuntimeException re)
        {
            throw re;
        }
        catch (Throwable t)
        {
            if (tunnelType.isAssignableFrom(te.getClass()))
            {
                @SuppressWarnings("unchecked")
                T typedT = (T)t;
                throw typedT;
            }
            else
            {
                throw new RuntimeException("Unexpected tunneled exception type.", t);
            }
        }
    }

    // @eclipseFormat:off
    public static <FromT, E extends Exception> void tunnel(
        Class<E> tunnelType,
        ExceptionThrowingConsumer<Consumer<? super FromT>, E> to,
        ExceptionThrowingConsumer<? super FromT, E> from) throws E
    // @eclipseFormat:on
    {
        untunnel(() -> to.accept(tunnel(from, tunnelType)), tunnelType);
    }

    // @eclipseFormat:off
    public static <T, E extends Exception> Consumer<T> tunnel(
        ExceptionThrowingConsumer<? super T, E> consumer, Class<E> tunnelType)
    // @eclipseFormat:on
    {
        if (consumer == null) throw new NullArgumentException("consumer");
        if (tunnelType == null) throw new NullArgumentException("tunnelType");

        return arg -> tunnelNow(() -> consumer.accept(arg), tunnelType);
    }

    public static <E extends Exception> void tunnelNow(ExceptionThrowingRunnable<E> runnable,
                                                       Class<E> tunnelType)
    {
        if (runnable == null) throw new NullArgumentException("runnable");
        if (tunnelType == null) throw new NullArgumentException("tunnelType");

        try
        {
            runnable.run();
        }
        catch (RuntimeException re)
        {
            // Runtime exceptions don't need to be tunneled.
            throw re;
        }
        catch (Throwable t)
        {
            if (tunnelType.isAssignableFrom(t.getClass()))
            {
                throw new TunneledException(t);
            }
            else
            {
                throw new RuntimeException("Unexpected exception.", t);
            }
        }
    }

    // @eclipseFormat:off
    public static <T, E extends Exception> void untunnel(
        ExceptionThrowingConsumer<? super T, E> consumer,
        T object,
        Class<E> tunnelType) throws E
    // @eclipseFormat:on
    {
        if (consumer == null) throw new NullArgumentException("consumer");
        if (tunnelType == null) throw new NullArgumentException("tunnelType");

        try
        {
            consumer.accept(object);
        }
        catch (TunneledException e)
        {
            rethrowTunneled(e, tunnelType);
        }
    }

    public static <E extends Exception> void untunnel(ExceptionThrowingRunnable<E> runnable,
                                                      Class<E> tunnelType) throws E
    {
        try
        {
            runnable.run();
        }
        catch (TunneledException e)
        {
            rethrowTunneled(e, tunnelType);
        }
    }

    private ExceptionHelper()
    {
        throw new UnsupportedOperationException("instantiating a utility class.");
    }
}
