/*
 * Copyright (C) 2014, All Rights Reserved, by Formation Data Systems, Inc.
 *
 *  This software is furnished under a license and may be used and copied only
 *  in  accordance  with  the  terms  of such  license and with the inclusion
 *  of the above copyright notice. This software or  any  other copies thereof
 *  may not be provided or otherwise made available to any other person.
 *  No title to and ownership of  the  software  is  hereby transferred.
 *
 *  The information in this software is subject to change without  notice
 *  and  should  not be  construed  as  a commitment by Formation Data Systems.
 *
 *  Formation Data Systems assumes no responsibility for the use or  reliability
 *  of its software on equipment which is not supplied by Formation Date Systems.
 */

package com.formationds.commons.util.resource;

/**
 * @author ptinius
 */
@SuppressWarnings( { "unused" } )
public class ResourceException
  extends Exception
{
  /**
   * Constructs a new exception with <code>null</code> as its detail message.
   * The cause is not initialized, and may subsequently be initialized by a call
   * to {@link #initCause}.
   */
  public ResourceException()
  {
    super();
  }

  /**
   * Constructs a new exception with the specified cause and a detail message of
   * <tt>(cause==null ? null : cause.toString())</tt> (which typically contains
   * the class and detail message of <tt>cause</tt>). This constructor is useful
   * for exceptions that are little more than wrappers for other throwables (for
   * example, {@link java.security.PrivilegedActionException}).
   *
   * @param cause the cause (which is saved for later retrieval by the {@link
   * #getCause()} method).  (A <tt>null</tt> value is permitted, and indicates
   * that the cause is nonexistent or unknown.)
   *
   * @since 1.4
   */
  public ResourceException(final Throwable cause)
  {
    super( cause );
  }

  /**
   * Constructs a new exception with the specified detail message.  The cause is
   * not initialized, and may subsequently be initialized by a call to {@link
   * #initCause}.
   *
   * @param message the detail message. The detail message is saved for later
   * retrieval by the {@link #getMessage()} method.
   */
  public ResourceException(final String message)
  {
    super( message );
  }

  /**
   * Constructs a new exception with the specified detail message and cause.
   * <p>Note that the detail message associated with <code>cause</code> is
   * <i>not</i> automatically incorporated in this exception's detail message.
   *
   * @param message the detail message (which is saved for later retrieval by
   * the {@link #getMessage()} method).
   * @param cause the cause (which is saved for later retrieval by the {@link
   * #getCause()} method).  (A <tt>null</tt> value is permitted, and indicates
   * that the cause is nonexistent or unknown.)
   *
   * @since 1.4
   */
  public ResourceException(final String message, final Throwable cause)
  {
    super( message,
           cause );
  }
}
