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

package com.formationds.commons.model.builder;

import com.formationds.commons.model.Metadata;

/**
 * @author ptinius
 */
public class MetadataBuilder {
  private long timestamp;
  private String volume;
  private String key;
  private String value;

  /**
   * default constructor
   */
  public MetadataBuilder() {
  }

  /**
   * @param timestamp the {2code long} representing the timestamp of the metadata
   *
   * @return Returns the {@link MetadataBuilder}
   */
  public MetadataBuilder withTimestamp( long timestamp ) {
    this.timestamp = timestamp;
    return this;
  }

  /**
   * @param volume the {@link String} representing the volume name
   * @return Returns the {@link MetadataBuilder}
   */
  public MetadataBuilder withVolume( String volume ) {
    this.volume = volume;
    return this;
  }

  /**
   * @param key the {@link String} representing the metadata key.
   *
   * @return Returns the {@link MetadataBuilder}
   */
  public MetadataBuilder withKey( String key ) {
    this.key = key;
    return this;
  }

  /**
   * @param value the {@link String} representing the value
   *
   * @return Returns the {@link MetadataBuilder}
   */
  public MetadataBuilder withValue( String value ) {
    this.value = value;
    return this;
  }

  /**
   * @return Returns the {@link Metadata}
   */
  public Metadata build() {
    Metadata metadata = new Metadata();
    metadata.setTimestamp( timestamp );
    metadata.setVolume( volume );
    metadata.setKey( key );
    metadata.setValue( value );
    return metadata;
  }
}
