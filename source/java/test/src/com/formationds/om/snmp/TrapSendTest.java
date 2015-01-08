/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.snmp;

import com.formationds.commons.events.EventCategory;
import com.formationds.commons.events.EventSeverity;
import com.formationds.commons.events.EventType;
import com.formationds.commons.model.entity.Event;
import com.formationds.commons.model.entity.SystemActivityEvent;
import org.junit.Before;
import org.junit.Test;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

public class TrapSendTest {

    static final Event mockedEvent = mock( SystemActivityEvent.class );


    @Before
    public void setUp( )
        throws Exception {

        when( mockedEvent.getCategory() ).thenReturn( EventCategory.SYSTEM );
        when( mockedEvent.getSeverity() ).thenReturn( EventSeverity.INFO );
        when( mockedEvent.getType() ).thenReturn( EventType.SYSTEM_EVENT );
        when( mockedEvent.getDefaultMessage() ).thenReturn( "One argument and its called '{0}'" );
        when( mockedEvent.getMessageArgs() ).thenReturn( new Object[] { "Arg1" } );

        System.setProperty( TrapSend.SNMP_TARGET_KEY, "127.0.0.1" );
    }

    @Test
    public void testNotify( )
        throws Exception {

        new TrapSend().notify( mockedEvent );

    }
}