/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.exception;

import com.formationds.commons.model.i18n.ModelResource;

/**
 * @author ptinius
 */
public class ParseException
  extends Exception {

  private static final long serialVersionUID = -2857747796871927312L;

  private static final String MESSAGE_FMT =
    ModelResource.getString( "ical.parse.error" );

  private final String errorOn;

  /**
   * Constructs a new exception with {@code null} as its detail message. The
   * cause is not initialized, and may subsequently be initialized by a call to
   * {@link #initCause}.
   */
  public ParseException( String errorOn ) {
    this.errorOn = errorOn;
  }

  /**
   * Constructs a new exception with the specified detail message.  The cause is
   * not initialized, and may subsequently be initialized by a call to {@link
   * #initCause}.
   *
   * @param message the detail message. The detail message is saved for later
   *                retrieval by the {@link #getMessage()} method.
   * @param errorOn
   */
  public ParseException( String message, String errorOn ) {
    super( message );
    this.errorOn = errorOn;
  }

  /**
   * Constructs a new exception with the specified detail message and cause.
   * <p>Note that the detail message associated with {@code cause} is <i>not</i>
   * automatically incorporated in this exception's detail message.
   *
   * @param message the detail message (which is saved for later retrieval by the
   *                {@link #getMessage()} method).
   * @param cause   the cause (which is saved for later retrieval by the {@link
   *                #getCause()} method).  (A <tt>null</tt> value is permitted,
   *                and indicates that the cause is nonexistent or unknown.)
   *
   * @param errorOn the ical parse element that failed to parse
   * @since 1.4
   */
  public ParseException( String message, Throwable cause, String errorOn ) {
    super( message, cause );
    this.errorOn = errorOn;
  }

  /**
   * Returns the detail message string of this throwable.
   *
   * @return  the detail message string of this {@code Throwable} instance
   *          (which may be {@code null}).
   */
  @Override
  public String getMessage() {
    return String.format( MESSAGE_FMT, errorOn );
  }
}
