/*
 * Copyright (C) 2014, All Rights Reserved, by Formation Data Systems, Inc.
 *
 * This software is furnished under a license and may be used and copied only
 * in  accordance  with  the  terms  of such  license and with the inclusion
 * of the above copyright notice. This software or  any  other copies thereof
 * may not be provided or otherwise made available to any other person.
 * No title to and ownership of  the  software  is  hereby transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Formation Data Systems.
 *
 * Formation Data Systems assumes no responsibility for the use or  reliability
 * of its software on equipment which is not supplied by Formation Date Systems.
 */

package com.formationds.om.rest;

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import com.formationds.security.AuthenticationToken;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.xdi.ConfigurationServiceCache;
import com.formationds.xdi.Xdi;

/**
 * @author ptinius
 */
public abstract class OMRestBase
  implements RequestHandler
{
  private final Xdi xdi;
  private final FDSP_ConfigPathReq.Iface legacyConfigPath;
  private final AuthenticationToken token;
  private final ConfigurationServiceCache cachedConfig;

  /**
   * @param xdi the {@link com.formationds.xdi.Xdi}
   * @param token the {@link com.formationds.security.AuthenticationToken}
   */
  protected OMRestBase( final Xdi xdi,
                        final AuthenticationToken token )
  {
    this( xdi, null, token, null );
  }

  /**
   * @param cachedConfig the {@link ConfigurationServiceCache}
   */
  protected OMRestBase( final ConfigurationServiceCache cachedConfig )
  {
    this(  null, null, null, cachedConfig );
  }

  /**
   * @param xdi the {@link com.formationds.xdi.Xdi}
   * @param legacyConfigPath the {@link FDS_ProtocolInterface.FDSP_ConfigPathReq.Iface}
   * @param token the {@link com.formationds.security.AuthenticationToken}
   * @param cachedConfig the {@link ConfigurationServiceCache}
   */
  protected OMRestBase( final Xdi xdi,
                        final FDSP_ConfigPathReq.Iface legacyConfigPath,
                        final AuthenticationToken token,
                        final ConfigurationServiceCache cachedConfig )
  {
    super();

    this.xdi = xdi;
    this.legacyConfigPath = legacyConfigPath;
    this.token = token;
    this.cachedConfig = cachedConfig;
  }

  protected Xdi getXdi()
  {
    return xdi;
  }

  protected FDSP_ConfigPathReq.Iface getLegacyConfigPath()
  {
    return legacyConfigPath;
  }

  protected AuthenticationToken getToken()
  {
    return token;
  }

  protected ConfigurationServiceCache getConfigurationServiceCache() { return cachedConfig; }
}
