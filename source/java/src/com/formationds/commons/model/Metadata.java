/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;

/**
 * @author ptinius
 */
public class Metadata
  extends ModelBase
{
  private static final long serialVersionUID = -7637239107402084473L;

  private Long timestamp;
  private String volume;
  private String key;
  private String value;

  /**
   * default constructor
   */
  public Metadata()
  {
    super();
  }

  /**
   * @return Returns {@link Long} representing the timestamp, in milliseconds
   */
  public long getTimestamp()
  {
    return timestamp;
  }

  /**
   * @param timestamp the {@link Long} representing the timestamp, in milliseconds
   */
  public void setTimestamp( final Long timestamp )
  {
    this.timestamp = timestamp;
  }

  /**
   * @return Returns {@link String} representing the volume's name
   */
  public String getVolume()
  {
    return volume;
  }

  /**
   * @param volume the {@link String} representing the volume's name
   */
  public void setVolume( final String volume )
  {
    this.volume = volume;
  }

  /**
   * @return Returns {@link String} representing the key
   */
  public String getKey()
  {
    return key;
  }

  /**
   * @param key the {@link String} representing the key
   */
  public void setKey( final String key )
  {
    this.key = key;
  }

  /**
   * @return Returns {@link String} representing the value
   */
  public String getValue()
  {
    return value;
  }

  /**
   * @param value the {@link String} representing the value
   */
  public void setValue( final String value )
  {
    this.value = value;
  }
}
