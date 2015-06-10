/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.snmp;

import com.formationds.commons.events.EventCategory;
import com.formationds.commons.events.EventSeverity;
import com.formationds.commons.model.entity.Event;
import com.formationds.commons.model.entity.SystemActivityEvent;
import com.google.common.base.Preconditions;
import org.snmp4j.CommunityTarget;
import org.snmp4j.PDU;
import org.snmp4j.Snmp;
import org.snmp4j.mp.SnmpConstants;
import org.snmp4j.smi.Address;
import org.snmp4j.smi.GenericAddress;
import org.snmp4j.smi.IpAddress;
import org.snmp4j.smi.OID;
import org.snmp4j.smi.OctetString;
import org.snmp4j.smi.TimeTicks;
import org.snmp4j.smi.VariableBinding;
import org.snmp4j.transport.DefaultUdpTransportMapping;

import java.io.IOException;
import java.io.Serializable;
import java.net.InetAddress;
import java.time.Instant;

/**
 * @author ptinius
 */
public class TrapSend
    implements Serializable {

    private static final long serialVersionUID = -8266712950503719396L;

    // platform.conf configuration properties
    public static final String SNMP_TARGET_KEY = "fds.om.snmp.targetip";
    public static final String SNMP_COMMUNITY_KEY = "fds.om.snmp.community";

    private static final String DEF_COMMUNITY = "public";
    private static final String DEF_SNMP_TARGET_ADDRESS = "udp:%s/162";

    private static final OID ENTERPRISE_SPECIFIC =
        new OID( "1.3.6.1.6.3.1.1.5.6" );

    private final Snmp snmp;
    private final String target;
    private final String community;

    private final String sendIpAddress;
    private final long uptime;

    public TrapSend( )
        throws IOException {

        this.target = System.getProperty( SNMP_TARGET_KEY );
        this.community = System.getProperty( SNMP_COMMUNITY_KEY,
                                             DEF_COMMUNITY );

        this.sendIpAddress = InetAddress.getLocalHost()
                                        .getHostAddress();
        this.uptime = Instant.now().getEpochSecond();

        Preconditions.checkNotNull( target );

        this.snmp = new Snmp( new DefaultUdpTransportMapping() );
        this.snmp.listen();

    }

    protected CommunityTarget target( ) {

        final Address targetAddress =
            GenericAddress.parse( String.format( DEF_SNMP_TARGET_ADDRESS,
                                                 target ) );

        final CommunityTarget target = new CommunityTarget();

        target.setCommunity( new OctetString( community ) );
        target.setAddress( targetAddress );
        target.setVersion( SnmpConstants.version2c );
        target.setRetries( 2 );
        target.setTimeout( 5000 );

        return target;

    }

    protected PDU pdu( final Event event ) {

        final PDU pdu = new PDU();
        pdu.setType( PDU.NOTIFICATION );

        pdu.add( new VariableBinding( SnmpConstants.sysUpTime,
                                      new TimeTicks( upTime() ) ) );
        pdu.add( new VariableBinding( SnmpConstants.snmpTrapOID,
                                      ENTERPRISE_SPECIFIC ) );

        pdu.add( new VariableBinding( SnmpConstants.snmpTrapOID,
                                      FDSOid.DS_OID ) );
        pdu.add( new VariableBinding( FDSOid.EVENT_TIME_OID,
                                      new TimeTicks( Instant.now()
                                                            .getEpochSecond() ) ) );
        pdu.add( new VariableBinding( FDSOid.EVENT_SERVICE_NAME_OID,
                                      new OctetString( "OM" ) ) );
        pdu.add( new VariableBinding( SnmpConstants.snmpTrapAddress,
                                      new IpAddress( sendIpAddress ) ) );
        pdu.add( new VariableBinding( FDSOid.EVENT_TYPE_OID,
                                      new OctetString( event.getType()
                                                            .name() ) ) );
        pdu.add( new VariableBinding( FDSOid.EVENT_CATEGORY_OID,
                                      new OctetString( event.getCategory()
                                                            .name() ) ) );
        pdu.add( new VariableBinding( FDSOid.EVENT_SEVERITY_OID,
                                      new OctetString( event.getSeverity()
                                                            .name() ) ) );
        pdu.add( new VariableBinding( FDSOid.EVENT_MESSAGE_OID,
                                      new OctetString(
                                          String.format( event.getDefaultMessage(),
                                                         event.getMessageArgs())) ) );

        return pdu;

    }

    protected long upTime() {

        return this.uptime;

    }

    public void notify( final Event event )
        throws IOException {

        snmp.notify( pdu( event ), target() );

    }

    // TODO finish additional send ( trap, inform, get, set, etc. )

    public static void main( final String[] args ) {

        try {

            System.setProperty( SNMP_TARGET_KEY, "127.0.0.1" );
            TrapSend trap = new TrapSend();
            final SystemActivityEvent event =
                new SystemActivityEvent( EventCategory.SYSTEM,
                                         EventSeverity.INFO,
                                         "This is '%s' and '%s'",
                                         "messageKey",
                                         "Arg1", "arg2" );

            trap.notify( event );

        } catch( IOException e ) {

            e.printStackTrace();

        }

    }
}
