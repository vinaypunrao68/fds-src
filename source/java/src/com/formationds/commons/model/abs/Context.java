/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.abs;

/**
 * @author ptinius
 */
public abstract class Context
  extends ModelBase {

  private static final long serialVersionUID = 2523968528912372502L;

  /**
   * @return Returns a {@link T} representing the context
   */
  public abstract <T> T getContext();
}
