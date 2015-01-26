/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;
import com.formationds.commons.model.type.ManagerType;
import com.formationds.commons.model.type.ServiceStatus;

/**
 * @author ptinius
 */
public class Service
  extends ModelBase {
  private static final long serialVersionUID = -1577170593630479004L;

  private Long uuid;

  private String autoName;
  private Integer port;
  private ServiceStatus status;
  private ManagerType type;

  protected Service( ) {
  }

  public String getAutoName( ) {
    return autoName;
  }

  public Long getUuid( ) {
    return uuid;
  }

  public ManagerType getType( ) {
    return type;
  }

  public ServiceStatus getStatus( ) {
    return status;
  }

  public Integer getPort( ) {
    return port;
  }

  public static IAutoName uuid( final Long uuid ) {
    return new Service.Builder( uuid );
  }

  public interface IAutoName {
    IPort autoName( final String autoName );
  }

  public interface IPort {
    IStatus port( final Integer port );
  }

  public interface IStatus {
    IType status( final ServiceStatus status );
  }

  public interface IType {
    IBuild type( final ManagerType type );
  }

  public interface IBuild {
    Service build( );
  }

  private static class Builder
      implements IAutoName, IPort, IStatus, IType, IBuild {
    private Service instance = new Service();

    private Builder( final Long uuid ) {
      instance.uuid = uuid;
    }

    @Override
    public IPort autoName( final String autoName ) {
      instance.autoName = autoName;
      return this;
    }

    @Override
    public IStatus port( final Integer port ) {
      instance.port = port;
      return this;
    }

    @Override
    public IType status( final ServiceStatus status ) {
      instance.status = status;
      return this;
    }

    @Override
    public IBuild type( final ManagerType type ) {
      instance.type = type;
      return this;
    }

    public Service build( ) {
      return instance;
    }
  }
}
