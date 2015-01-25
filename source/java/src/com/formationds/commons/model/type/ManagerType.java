/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.type;

import com.formationds.commons.model.i18n.ModelResource;

/**
 * @author ptinius
 */
public enum ManagerType
{
  FDSP_PLATFORM( ModelResource.getString( "fdsp.platform.manager.displayable" ) ),
  FDSP_STOR_MGR( ModelResource.getString( "fdsp.storage.manager.displayable" ) ),
  FDSP_DATA_MGR( ModelResource.getString( "fdsp.data.manager.displayable" ) ),
  FDSP_STOR_HVISOR( ModelResource.getString( "fdsp.storage.hypervisor.displayable" ) ),
  FDSP_ORCH_MGR( ModelResource.getString( "fdsp.orchestrator.manager.displayable" ) ),
  FDSP_CLI_MGR( ModelResource.getString( "fdsp.cli.manager.displayable" ) ),
  FDSP_OMCLIENT_MGR( ModelResource.getString( "fdsp.om.client.manager.displayable" ) ),
  FDSP_MIGRATION_MGR( ModelResource.getString( "fdsp.migration.manager.displayable" ) ),
  FDSP_PLATFORM_SVC( ModelResource.getString( "fdsp.platform.server.displayable" ) ),
  FDSP_METADATASYNC_HDR( ModelResource.getString( "fdsp.metadata.sync.hdr.displayable" ) ),
  FDSP_TEST_APP( ModelResource.getString( "fdsp.test.app.displayable" ) ),
  UNKNOWN( "known FDSP Manager type" );

  private String localized;

  /**
   * @param localized the localized name
   */
  ManagerType( final String localized )
  {
    this.localized = localized;
  }

  /**
   * @return Returns the displayable name
   */
  public String getLocalizedName() { return localized; }

  /**
   * @param name the {@link String} representing the name
   *
   * @return Returns {@link ManagerType} representing the node service type
   */
  public static ManagerType lookup( final String name )
  {
    for( final ManagerType type : values() )
    {
      if( type.name().equalsIgnoreCase( name ) )
      {
        return type;
      }
    }

    return UNKNOWN;
  }

  public static ManagerType lookup( final int ordinal )
  {
    for( final ManagerType type : values() )
    {
      if( type.ordinal() == ordinal )
      {
        return type;
      }
    }

    return UNKNOWN;
  }
}
