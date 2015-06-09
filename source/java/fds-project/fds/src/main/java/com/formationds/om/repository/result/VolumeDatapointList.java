/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.result;

import com.formationds.commons.crud.SearchResults;
import com.formationds.commons.model.entity.VolumeDatapoint;

import java.util.ArrayList;

/**
 * @author ptinius
 */
public class VolumeDatapointList
  extends ArrayList<VolumeDatapoint>
  implements SearchResults {
  private static final long serialVersionUID = 6323732602300266521L;
}
