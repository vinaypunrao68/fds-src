/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;

/**
 * @author ptinius
 */
public class ExceptionHelper {
  private static final transient Logger logger =
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
}
