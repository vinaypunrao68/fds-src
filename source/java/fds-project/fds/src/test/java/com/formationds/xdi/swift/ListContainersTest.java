/**
 * Copyright (c) 2014 Formation Data Systems.  All rights reserved.
 */

package com.formationds.xdi.swift;

import com.formationds.apis.VolumeDescriptor;
import com.formationds.apis.VolumeSettings;
import com.formationds.apis.VolumeStatus;
import com.formationds.om.helper.SingletonAmAPI;
import com.formationds.security.AuthenticationToken;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.AsyncAm;
import com.formationds.xdi.Xdi;
import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;
import org.w3c.dom.Document;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CompletableFuture;

import static org.mockito.Matchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

public class ListContainersTest {

    static final AsyncAm mockedAMService = mock( AsyncAm.class );
    static final Xdi mockedXdi = mock( Xdi.class );
    static final AuthenticationToken mockedToken = mock( AuthenticationToken.class );

    @BeforeClass
    static public void setUpClass() throws Exception {

        SingletonAmAPI.instance().api( mockedAMService );
        VolumeStatus vstat = new VolumeStatus();
        vstat.setCurrentUsageInBytes( 1024 );
        vstat.setBlobCount( 100 );

        when( mockedXdi.statVolume( any(), any(),  any()) ).thenReturn( CompletableFuture
                                                                                  .completedFuture( vstat ) );
    }

    @Test
    public void testXmlResource () throws Exception {
        List<VolumeDescriptor> volumes = new ArrayList<>();
        String accountName = "anAccount";

        int max = 5;
        for (int i = 0; i < max; i++) {
            VolumeDescriptor v = new VolumeDescriptor();

            v.setName( "v" + i );
            v.setVolId( i );
            v.setDateCreated( Instant.now().toEpochMilli() );
            v.setTenantId( 0 );
            VolumeSettings settings = new VolumeSettings();
            v.setPolicy( settings );

            volumes.add( v );
        }

        ListContainers lc = new ListContainers( mockedXdi, mockedToken );

        Resource xmlResource = lc.xmlView( volumes, accountName );
        System.out.println( "---- W3CDOM ----" );
        ByteArrayOutputStream out = new ByteArrayOutputStream( 1024 );
        xmlResource.render( out );

        DocumentBuilder builder = DocumentBuilderFactory.newInstance().newDocumentBuilder();
        Document d = builder.parse( new ByteArrayInputStream( out.toByteArray() ) );

        NodeList l = d.getChildNodes();
        Assert.assertEquals( 1, l.getLength() );

        Node account = l.item( 0 );
        NodeList containers = d.getElementsByTagName( "container" );
        for ( int n = 0; n < containers.getLength(); n++) {
            Node container = containers.item( n );
            System.out.println( container.getNodeName() );

            NodeList cc = container.getChildNodes();
            for (int x = 0; x < cc.getLength(); x++) {
                Node cinfo = cc.item( x );
                if (cinfo.getNodeType() != Node.TEXT_NODE) {
                    System.out.println( "\t" + cinfo.getNodeName() + ":" + cinfo.getTextContent() );
                    switch ( cinfo.getNodeName() ) {
                        case "name":
                            Assert.assertEquals( "Expecting v" + n, "v" + n, cinfo.getTextContent() );
                            break;
                        case "count":
                            Assert.assertEquals( "Expecting 100", "100", cinfo.getTextContent() );
                            break;
                        case "bytes":
                            Assert.assertEquals( "Expecting 1024", "1024", cinfo.getTextContent() );
                            break;
                        default:
                            Assert.fail( "Unexpected node " + cinfo.getNodeName() );
                    }
                }
            }
        };
        Assert.assertEquals( max, containers.getLength() );

//        System.out.println( "---- DOM4J ----");
//        dom4jResource.render( System.out );
    }

    /*
<?xml version="1.0" encoding="UTF-8"?>

<account name="anAccount">
  <container>
    <name>v0</name>
    <count>100</count>
    <bytes>1024</bytes>
  </container>
  <container>
    <name>v1</name>
    <count>100</count>
    <bytes>1024</bytes>
  </container>
  <container>
    <name>v2</name>
    <count>100</count>
    <bytes>1024</bytes>
  </container>
  <container>
    <name>v3</name>
    <count>100</count>
    <bytes>1024</bytes>
  </container>
  <container>
    <name>v4</name>
    <count>100</count>
    <bytes>1024</bytes>
  </container>
</account>
     */

}
