/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.util;

import com.formationds.commons.model.intf.IRecurrenceRuleValidator;

/**
 * @author ptinius
 */
public class RRIntegerValidator
  implements IRecurrenceRuleValidator<Integer> {

  private final Integer minimum;
  private final Integer maximum;
  private final Boolean allowed;

  /**
   * @param minimum the {@link Integer} representing the minimum value allowed
   * @param maximum the {@link Integer} representing the maximum value allowed
   * @param allowed the {@link Boolean} representing if negative vales are
   *                allowed
   */
  public RRIntegerValidator( final Integer minimum,
                             final Integer maximum,
                             final Boolean allowed ) {
    this.minimum = minimum;
    this.maximum = maximum;
    this.allowed = allowed;
  }

  /**
   * @param value the {@link Integer} representing the object to be validated
   *
   * @return Returns {@code true} if valid
   */
  @Override
  public boolean isValid( final Integer value ) {
    return ( value >= minimum ) &&
      ( value <= maximum ) &&
      ( allowed == ( value >= 0 ) );
  }
}
