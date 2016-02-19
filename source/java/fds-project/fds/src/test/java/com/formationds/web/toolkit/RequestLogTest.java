package com.formationds.web.toolkit;

import static org.junit.Assert.*;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

public class RequestLogTest {

    @BeforeClass
    public static void setUpBeforeClass() throws Exception {
    }

    @AfterClass
    public static void tearDownAfterClass() throws Exception {
    }

    @Before
    public void setUp() throws Exception {
    }

    @After
    public void tearDown() throws Exception {
    }

    @Test
    public void testFilterQueryParameters() {
        String queryString = "user=admin&password=admin&someparam=someval";
        String expected    = "user=admin&password=*****&someparam=someval";

        String filtered = RequestLog.filterQueryParameters( queryString, "password", "pwd", "p" );
        assertEquals( expected, filtered );

        queryString = "user=admin&someparam=someval&password=admin";
        expected    = "user=admin&someparam=someval&password=*****";

        filtered = RequestLog.filterQueryParameters( queryString, "password", "pwd", "p" );
        assertEquals( expected, filtered );

    }


}
