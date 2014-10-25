/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.result;

import com.formationds.commons.model.abs.ModelBase;

import java.util.Date;

/**
 * @author ptinius
 */
public class VolumeFirebreak
  extends ModelBase {
  private static final long serialVersionUID = 8947104246432649077L;

  private final String volumeName;
  private final String volumeId;
  private final Date lastOccurred;

  /**
   * @param volumeName   the {@link String} representing the volume name
   * @param volumeId     the {@link String} representing the unique volume id
   * @param lastOccurred the {@link Date} of when firebreak originally
   *                     occurred
   */
  public VolumeFirebreak( final String volumeName,
                          final String volumeId,
                          final Date lastOccurred ) {
    super();

    this.volumeName = volumeName;
    this.volumeId = volumeId;
    this.lastOccurred = lastOccurred;
  }

  /**
   * @return Returns the {@link String} representing the volume name
   */
  public String getVolumeName() {
    return volumeName;
  }

  /**
   * @return Returns the {@link Long} representing the unique volume id
   */
  public String getVolumeId() {
    return volumeId;
  }

  /**
   * @return Returns the {@link Date} of when firebreak originally occurred
   */
  public Date getLastOccurred() {
    return lastOccurred;
  }
}
