/*
 * Copyright (C) 2014, All Rights Reserved, by Formation Data Systems, Inc.
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

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;

/**
 * @author ptinius
 */
public class Metadata
  extends ModelBase
{
  private static final long serialVersionUID = -7637239107402084473L;

  private long timestamp;
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
   * @return Returns {@code long} representing the timestamp, in milliseconds
   */
  public long getTimestamp()
  {
    return timestamp;
  }

  /**
   * @param timestamp the {@code long} representing the timestamp, in milliseconds
   */
  public void setTimestamp( final long timestamp )
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
