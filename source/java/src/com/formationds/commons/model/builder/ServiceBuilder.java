/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.Service;
import com.formationds.commons.model.type.ManagerType;

/**
 * @author ptinius
 */
public class ServiceBuilder {
  private long uuid = -1L;
  private int controlPort = -1;
  private int migrationPort = -1;
  private int dataPort = -1;
  private int metasyncPort = -1;
  private ManagerType type = ManagerType.UNKNOWN;

  /**
   * default constructor
   */
  public ServiceBuilder() {
  }

  /**
   * @return Returns the {@link ServiceBuilder}
   */
  public static ServiceBuilder aService() {
    return new ServiceBuilder();
  }

  /**
   * @param uuid the {@link String} representing the universal unique identifier
   *
   * @return Returns the {@link ServiceBuilder}
   */
  public ServiceBuilder withUuid( long uuid ) {
    this.uuid = uuid;
    return this;
  }

  /**
   * @param controlPort the {@code int} representing the listening control port
   *
   * @return Returns the {@link ServiceBuilder}
   */
  public ServiceBuilder withControlPort( int controlPort ) {
    this.controlPort = controlPort;
    return this;
  }

  /**
   * @param migrationPort the {@code int} representing the listening migration port
   *
   * @return Returns the {@link ServiceBuilder}
   */
  public ServiceBuilder withMigrationPort( int migrationPort ) {
    this.migrationPort = migrationPort;
    return this;
  }

  /**
   * @param dataPort the {@code int} representing the listening data port
   *
   * @return Returns the {@link ServiceBuilder}
   */
  public ServiceBuilder withDataPort( int dataPort ) {
    this.dataPort = dataPort;
    return this;
  }

  /**
   * @param metasyncPort the {@code int} representing the listening metasync port
   *
   * @return Returns the {@link ServiceBuilder}
   */
  public ServiceBuilder withMetasyncPort( int metasyncPort ) {
    this.metasyncPort = metasyncPort;
    return this;
  }

  /**
   * @param type the {@link ManagerType}
   *
   * @return Returns the {@link ServiceBuilder}
   */
  public ServiceBuilder withType( ManagerType type ) {
    this.type = type;
    return this;
  }

  /**
   * @return Returns the {@link ServiceBuilder}
   */
  public ServiceBuilder but() {
    return aService().withUuid( uuid )
                     .withControlPort( controlPort )
                     .withMigrationPort( migrationPort )
                     .withDataPort( dataPort )
                     .withMetasyncPort( metasyncPort )
                     .withType( type );
  }

  /**
   * @return Returns the {@link Service}
   */
  public Service build() {
    Service service = new Service();
    service.setUuid( uuid );
    service.setControlPort( controlPort );
    service.setMigrationPort( migrationPort );
    service.setDataPort( dataPort );
    service.setMetasyncPort( metasyncPort );
    service.setType( type );
    return service;
  }
}
