/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.type;

import com.formationds.commons.model.i18n.ModelResource;

/**
 * @author ptinius
 */
public enum NodeSvcMask
{
  NODE_SVC_SM( 1, ModelResource.getString( "node.svc.storage.manager" ) ),
  NODE_SVC_DM( 2, ModelResource.getString( "node.svc.data.manager" ) ),
  NODE_SVC_AM( 4, ModelResource.getString( "node.svc.access.manager" ) ),
  NODE_SVC_OM( 8, ModelResource.getString( "node.svc.orchestrator.manager" ) ),
  NODE_SVC_GENERAL( 4096, ModelResource.getString( "node.svc.general.manager" ) ),
  UNKNOWN( 8192, "known Node Service type" );

  private int value;
  private String displayable;

  NodeSvcMask( final int value, final String displayable )
  {
    this.value = value;
    this.displayable = displayable;
  }

  /**
   * @return Returns the value
   */
  public int getValue() { return value; }

  /**
   * @return Returns the displayable name
   */
  public String getDisplayable() { return displayable; }

  /**
   * @param name the {@link String} representing the name
   *
   * @return Returns {@link NodeSvcMask} representing the node service type
   */
  public static NodeSvcMask byName( final String name )
  {
    for( final NodeSvcMask type : values() )
    {
      if( type.name().equalsIgnoreCase( name ) )
      {
        return type;
      }
    }

    return UNKNOWN;
  }
}
