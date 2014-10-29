/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.helper;

import com.formationds.xdi.ConfigurationApi;

/**
 * @author ptinius
 */
public class SingletonConfigAPI {
  private static SingletonConfigAPI ourInstance = new SingletonConfigAPI();

  /**
   * singleton instance of SingletonConfigAPI
   */
  public static SingletonConfigAPI instance() {
    return ourInstance;
  }

  /**
   * singleton constructor
   */
  private SingletonConfigAPI() {
  }

  private ConfigurationApi api;

  /**
   * @return Returns the {@link ConfigurationApi}
   */
  public ConfigurationApi get() {
    return api;
  }

  /**
   * @param api the {@link ConfigurationApi}
   */
  public void set( final ConfigurationApi api ) {
    this.api = api;
  }
}
