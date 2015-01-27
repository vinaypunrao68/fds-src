/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;
import com.formationds.commons.model.type.NodeState;
import com.formationds.commons.model.type.ServiceType;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * @author ptinius
 */
public class Node
  extends ModelBase {

  private static final long serialVersionUID = -6684434195412037211L;

  private Integer id;         // used for persistent storage
  private String name;        // node name
  private Long uuid;          // node uuid
  private String ipV6address;
  private String ipV4address;

  private NodeState state;    // node state

  private Map<ServiceType, List<Service>> services = new HashMap<ServiceType, List<Service>>();

  private Node( ) {
  }

  public static IIpV6address uuid( final Long uuid ) {
    return new Node.Builder( uuid );
  }

  public Map<ServiceType, List<Service>> getServices() {
    return services;
  }
  
  public void addService( Service service ){
	  
	  ServiceType serviceType = ServiceType.valueOf( service.getAutoName() );
	  List<Service> serviceList = getServices().get( serviceType );
	  
	  if ( serviceList == null ){
		  serviceList = new ArrayList<Service>();
	  }
	  
	  if ( !serviceList.contains( service ) ) {
		  serviceList.add( service );
	  }
	  
	  getServices().put( serviceType,  serviceList );
  }

  public interface IIpV6address {
    IIpV4address ipV6address( final String ipV6address );
  }

  public interface IIpV4address {
    IBuild ipV4address( final String ipV4address );
  }

  public interface IBuild {
    IBuild id( final Integer id );

    IBuild name( final String name );

    IBuild state( final NodeState state );

    Node build( );
  }

  private static class Builder
      implements IIpV6address, IIpV4address, IBuild {
    private Node instance = new Node();

    private Builder( final Long uuid ) {
      instance.uuid = uuid;
    }

    @Override
    public IIpV4address ipV6address( final String ipV6address ) {
      instance.ipV6address = ipV6address;
      return this;
    }

    @Override
    public IBuild ipV4address( final String ipV4address ) {
      instance.ipV4address = ipV4address;
      return this;
    }

    @Override
    public IBuild id( final Integer id ) {
      instance.id = id;
      return this;
    }

    @Override
    public IBuild name( final String name ) {
      instance.name = name;
      return this;
    }

    @Override
    public IBuild state( final NodeState state ) {
      instance.state = state;
      return this;
    }

    public Node build( ) {
      return instance;
    }
  }
}
