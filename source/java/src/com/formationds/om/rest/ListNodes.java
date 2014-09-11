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
import FDS_ProtocolInterface.FDSP_MsgHdrType;
import FDS_ProtocolInterface.FDSP_Node_Info_Type;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.datatype.jsonorg.JsonOrgModule;
import com.formationds.model.Node;
import com.formationds.model.ObjectFactory;
import com.formationds.model.type.ManagerType;
import com.formationds.model.type.NodeState;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * @author ptinius
 */
public class ListNodes implements RequestHandler
{
  private FDSP_ConfigPathReq.Iface configPathClient;

  public ListNodes( FDSP_ConfigPathReq.Iface configPathClient )
  {
    this.configPathClient = configPathClient;
  }

  public Resource handle( Request request, Map<String, String> routeParameters )
    throws Exception
  {
    List<FDSP_Node_Info_Type> list =
      configPathClient.ListServices( new FDSP_MsgHdrType() );
//    ObjectMapper mapper = new ObjectMapper();
//    mapper.registerModule( new JsonOrgModule() );
//    JSONArray jsonArray = mapper.convertValue( list, JSONArray.class );
//    for( int i = 0;
//         i < jsonArray.length();
//         i++ )
//    {
//      JSONObject jsonObject = jsonArray.getJSONObject( i );
//      long nodeUuid = jsonObject.getLong( "node_uuid" );
//      long serviceUuid = jsonObject.getLong( "service_uuid" );
//      jsonObject.put( "site", "Fremont" )
//                .put( "domain", "Formation Data Systems" )
//                .put( "local_domain", "Formation Data Systems" )
//                .put( "node_uuid", Long.toString( nodeUuid ) )
//                .put( "service_uuid", Long.toString( serviceUuid ) );
//    }
//    return new JsonResource( jsonArray );

    final List<Node> nodes = new ArrayList<>( );
    for( final FDSP_Node_Info_Type info : list )
    {
      final Node node = ObjectFactory.createNode();

      node.setId( info.node_id );
      node.setName( info.getNode_name() );
      node.setRoot( info.getNode_root() );
      node.setState( NodeState.valueOf( info.getNode_state()
                                            .name()
                                            .toUpperCase() ) );
      node.setType( ManagerType.valueOf( info.getNode_type()
                                             .name()
                                             .toUpperCase() ) );
      node.setUuid( Long.toString( info.getNode_uuid() ) );
      node.setSvc_uuid( Long.toString( info.getService_uuid() ) );

      // TODO determine where we need to pull these values from
      //  hopefully the answer isn't hardcoded!
      node.setSite( "Fremont" );
      node.setDomain( "Formation Data System, Inc." );
      node.setLocalDomain( "Formation Data System, Inc." );

      node.setControlPort( info.getControl_port() );
      node.setDataPort( info.getData_port() );
      node.setMetasyncPort( info.getMetasync_port() );
      node.setMigrationPort( info.getMigration_port() );

      node.setLoAddr( info.getIp_lo_addr() );
      node.setHiAddr( info.getIp_hi_addr() );

      nodes.add( node );
    }

    // simple? but doesn't into the "Object Model"
    ObjectMapper mapper = new ObjectMapper();
    mapper.registerModule( new JsonOrgModule() );

    return new JsonResource( mapper.convertValue( nodes, JSONArray.class ) );
  }

}
