/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util;

import java.io.Serializable;

/**
 * @author ptinius
 */
public interface NumbersValidator<E>
  extends Serializable {

  /**
   * @param value the {@link E} representing the object to be validated
   *
   * @return Returns {@code true} if valid
   */
  public boolean isValid( final E value );
}
