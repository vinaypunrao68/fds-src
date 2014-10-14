/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 *
 * This software is furnished under a license and may be used and copied only
 * in  accordance  with  the  terms  of such  license and with the inclusion
 * of the above copyright notice. This software or  any  other copies thereof
 * may not be provided or otherwise made available to any other person.
 * No title to and ownership of  the  software  is  hereby transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Formation Data Systems.
 *
 * Formation Data Systems assumes no responsibility for the use or  reliability
 * of its software on equipment which is not supplied by Formation Date Systems.
 */

package com.formationds.commons.model.exception;

/**
 * @author ptinius
 */
public class UnsupportedMetricException
  extends Exception {
  /**
   * Constructs a new exception with the specified detail message.  The cause is
   * not initialized, and may subsequently be initialized by a call to {@link
   * #initCause}.
   *
   * @param message the detail message. The detail message is saved for later
   *                retrieval by the {@link #getMessage()} method.
   */
  public UnsupportedMetricException( String message ) {
    super( message );
  }
}
