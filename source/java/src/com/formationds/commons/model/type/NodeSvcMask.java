/*
 * Copyright (C) 2014, All Rights Reserved, by Formation Data Systems, Inc.
 *
 *  This software is furnished under a license and may be used and copied only
 *  in  accordance  with  the  terms  of such  license and with the inclusion
 *  of the above copyright notice. This software or  any  other copies thereof
 *  may not be provided or otherwise made available to any other person.
 *  No title to and ownership of  the  software  is  hereby transferred.
 *
 *  The information in this software is subject to change without  notice
 *  and  should  not be  construed  as  a commitment by Formation Data Systems.
 *
 *  Formation Data Systems assumes no responsibility for the use or  reliability
 *  of its software on equipment which is not supplied by Formation Date Systems.
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
