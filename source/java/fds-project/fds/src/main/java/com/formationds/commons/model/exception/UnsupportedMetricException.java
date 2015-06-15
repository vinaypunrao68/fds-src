/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.exception;

/**
 * @author ptinius
 */
public class UnsupportedMetricException
  extends IllegalArgumentException {
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
