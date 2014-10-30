/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.util;

import com.formationds.commons.model.i18n.ModelResource;

/**
 * @author ptinius
 */
public class ModelFieldValidator {

  public enum KeyFields {
    SLA,
    LIMIT,
    PRIORITY;
  }

  private final KeyFields field;
  private final Long minValue;
  private final Long maxValue;

  /**
   * @param field    the {@link KeyFields} representing the field name
   * @param minValue the {@code Long} representing the minimum value allowed
   * @param maxValue the {@code Long} representing the maximum value allowed
   */
  public ModelFieldValidator( final KeyFields field, final Long minValue,
                              final Long maxValue ) {
    this.field = field;
    this.minValue = minValue;
    this.maxValue = maxValue;
  }

  /**
   * @return Returns {@code Long} representing the minimum value allowed
   */
  public Long getMinValue() {
    return minValue;
  }

  /**
   * @return Returns {@code Long} representing the maximum value allowed
   */
  public Long getMaxValue() {
    return maxValue;
  }

  /**
   * @return Returns {@link KeyFields} representing the field name
   */
  public KeyFields getField() {
    return field;
  }

  /**
   * @param value the {@code Integer} to be validated
   *
   * @return Returns {@code true} if is valid. Otherwise {@code false}
   */
  public boolean isValid( final Integer value ) {
    // if value is between min and max then valid, otherwise invalid
    return ( value >= getMinValue() ) && ( value <= getMaxValue() );
  }

  /**
   * @param value the {@code Long} to be validated
   *
   * @return Returns {@code true} if is valid. Otherwise {@code false}
   */
  public boolean isValid( final Long value ) {
    // if value is between min and max then valid, otherwise invalid
    return ( value >= getMinValue() ) && ( value <= getMaxValue() );
  }

  /**
   * @param validator the {@link ModelFieldValidator}
   *
   * @return Returns {@link String} representing the out-of-range message
   */
  public static String outOfRange( final ModelFieldValidator validator,
                                   final Long value ) {
    return String.format(
      ModelResource.getString( "model.validation.out.range" ),
      value,
      validator.getField(),
      validator.getMinValue(),
      validator.getMaxValue() );
  }
}
