/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.function.Consumer;
import java.util.function.Function;
import java.util.function.Supplier;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.TunneledException;
import com.formationds.commons.util.functional.ExceptionThrowingConsumer;
import com.formationds.commons.util.functional.ExceptionThrowingFunction;
import com.formationds.commons.util.functional.ExceptionThrowingRunnable;

// @eclipseFormat:off
/**
 * General exception handling utilities.
 * 
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

    /**
     * Get the tunneled exception if it was tunneled, otherwise pass the exception through.
     * 
     * @param t The exception to check.
     * @param tunnelType The type of exception that may be tunneled.
     * 
     * @return {@code t} if it is not a tunneled exception, or {@link Throwable#getCause()
     *         t.getCause()} if it is.
     */
    public static <T extends Throwable> Throwable getTunneledIfTunneled(Throwable t,
                                                                        Class<T> tunnelType)
    {
        if (t == null) throw new NullArgumentException("t");
        if (tunnelType == null) throw new NullArgumentException("tunnelType");

        if (t instanceof TunneledException)
        {
            return getTunneled((TunneledException)t, tunnelType);
        }
        else
        {
            return t;
        }
    }

    /**
     * Get the tunneled exception.
     * 
     * @param te The exception tunneling another exception.
     * @param tunnelType The type of exception that may be tunneled.
     * 
     * @return The tunneled exception.
     */
    public static <T extends Throwable> T getTunneled(TunneledException te, Class<T> tunnelType)
    {
        if (te == null) throw new NullArgumentException("te");
        if (tunnelType == null) throw new NullArgumentException("tunnelType");

        Throwable tunneled = te.getCause();
        Class<?> tunneledType = tunneled.getClass();
        if (tunnelType.isAssignableFrom(tunneledType))
        {
            @SuppressWarnings("unchecked")
            T typedTunneled = (T)tunneled;
            return typedTunneled;
        }
        else
        {
            throw new RuntimeException("Unexpected tunneled exception type.", tunneled);
        }
    }

    /**
     * Rethrow a tunneled exception.
     * 
     * @param te The exception tunneling another exception.
     * @param tunnelType The type of exception that may be tunneled.
     * 
     * @throws T always.
     */
    public static <T extends Throwable> void rethrowTunneled(TunneledException te,
                                                             Class<T> tunnelType) throws T
    {
        if (te == null) throw new NullArgumentException("te");
        if (tunnelType == null) throw new NullArgumentException("tunnelType");

        throw getTunneled(te, tunnelType);
    }

    /**
     * Tunnel an exception if it occurs.
     * 
     * @param tunnelType The type of exception to tunnel.
     * @param to The code that will receive tunneled exceptions.
     * @param from Code that may throw an exception of {@code tunnelType}.
     * 
     * @throws E when {@code from} throws.
     */
    // @eclipseFormat:off
    public static <FromT, E extends Exception> void tunnel(
        Class<E> tunnelType,
        ExceptionThrowingConsumer<Consumer<? super FromT>, E> to,
        ExceptionThrowingConsumer<? super FromT, E> from) throws E
    // @eclipseFormat:on
    {
        untunnel(() -> to.accept(tunnel(from, tunnelType)), tunnelType);
    }
    
    public static <InT, BetweenT, OutT, InE extends Exception, OutE extends Exception> OutT tunnel(
            Class<InE> inTunnelType,
            ExceptionThrowingFunction<Function<? super InT, ? extends BetweenT>, ? extends OutT, OutE> to,
            ExceptionThrowingFunction<? super InT, ? extends BetweenT, InE> from) throws InE, OutE
    {
        Function<InT, BetweenT> inner = tunnel(from, inTunnelType);
        
        return untunnel(to, inner, inTunnelType);
    }

    /**
     * Tunnel an exception if it occurs.
     * 
     * @param consumer Code that may throw an exception of {@code tunnelType}.
     * @param tunnelType The type of exception to tunnel.
     * 
     * @return A wrapped method that will not throw {@code tunnelType}.
     */
    // @eclipseFormat:off
    public static <T, E extends Exception> Consumer<T> tunnel(
        ExceptionThrowingConsumer<? super T, E> consumer, Class<E> tunnelType)
    // @eclipseFormat:on
    {
        if (consumer == null) throw new NullArgumentException("consumer");
        if (tunnelType == null) throw new NullArgumentException("tunnelType");

        return arg -> tunnelNow(() -> consumer.accept(arg), tunnelType);
    }

    public static <InT, BetweenT, OutT, E extends Exception> Function<InT, BetweenT> tunnel(
            ExceptionThrowingFunction<? super InT, ? extends BetweenT, ? extends E> func,
            Class<E> tunnelType)
    {
        return arg -> tunnelNow(inArg -> func.apply(inArg), arg, tunnelType);
    }
    
    /**
     * Immediately execute code and tunnel an exception.
     * 
     * @param runnable The code to execute that may throw {@code tunnelType}.
     * @param tunnelType The type of exception to tunnel.
     */
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

    public static <InT, BetweenT, OutT, E extends Exception> BetweenT tunnelNow(
            ExceptionThrowingFunction<? super InT, ? extends BetweenT, ? extends E> inner,
            InT argument,
            Class<E> tunnelType)
    {
        try
        {
            return inner.apply(argument);
        }
        catch (RuntimeException re)
        {
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
    
    /**
     * Throw the original exception after executing tunneled code.
     * 
     * @param consumer The code to execute that may throw a tunneled exception.
     * @param object An argument.
     * @param tunnelType Tunnel exceptions of this type.
     * 
     * @throws E when a tunneled exception holding this type is thrown.
     */
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

    /**
     * Throw the original exception after executing tunneled code.
     * 
     * @param runnable The code to execute that may throw a tunneled exception.
     * @param tunnelType Tunnel exceptions of this type.
     * 
     * @throws E when a tunneled exception holding this type is thrown.
     */
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
    
    public static <OutT, E extends Exception> OutT untunnel(
            Supplier<? extends OutT> outer,
            Class<E> tunnelType) throws E
    {
        try
        {
            return outer.get();
        }
        catch (TunneledException e)
        {
            rethrowTunneled(e, tunnelType);
        }
        // FIXME: Shouldn't be necessary.
        return null;
    }
    
    public static <InT, BetweenT, OutT, InE extends Exception, OutE extends Exception> OutT untunnel(
            ExceptionThrowingFunction<Function<? super InT, ? extends BetweenT>,
                                      ? extends OutT,
                                      ? extends OutE> outer,
            Function<? super InT, ? extends BetweenT> inner,
            Class<InE> tunnelType) throws InE, OutE
    {
        try
        {
            return outer.apply(inner);
        }
        catch (TunneledException e)
        {
            rethrowTunneled(e, tunnelType);
        }
        // FIXME: Shouldn't be necessary.
        return null;
    }

    /**
     * Prevent instantiation.
     */
    private ExceptionHelper()
    {
        throw new UnsupportedOperationException("instantiating a utility class.");
    }
}
