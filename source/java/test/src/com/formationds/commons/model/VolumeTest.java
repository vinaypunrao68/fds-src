/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.helper.ObjectModelHelper;
import com.google.gson.reflect.TypeToken;
import org.junit.Test;

import java.lang.reflect.Type;

public class VolumeTest {

  @Test
  public void test() {
    final Type TYPE =
      new TypeToken<Volume>() {
      }.getType();
    final String nater = "{\"priority\":10,\"sla\":0,\"limit\":300,\"snapshotPolicies\":[],\"name\":\"test_clone2\",\"data_connector\":{\"type\":\"Object\",\"api\":\"S3, Swift\"},\"id\":3051761627594267600}";
    final Volume volume = ObjectModelHelper.toObject( nater, TYPE );
    System.out.println( volume );
  }
}