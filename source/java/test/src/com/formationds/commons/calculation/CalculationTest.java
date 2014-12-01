package com.formationds.commons.calculation;

import com.formationds.commons.calculation.Calculation;
import org.junit.Assert;
import org.junit.Test;
import org.junit.Before;
import org.junit.After;

/**
 * Calculation Tester.
 *
 * @author <Authors name>
 * @version 1.0
 * @since <pre>Nov 23, 2014</pre>
 */
public class CalculationTest {

    @Before
    public void before() throws Exception {
    }

    @After
    public void after() throws Exception {
    }

    /**
     * Method: isFirebreak(final Double shortTerm, final Double longTerm)
     */
    @Test
    public void testIsFirebreak() throws Exception {
        Assert.assertTrue("Firebreak expecated (sigma ration is 5)", Calculation.isFirebreak(15.0D, 3.0D));
        Assert.assertFalse("Firebreak not expected", Calculation.isFirebreak(15.0D, 10.0D));
        Assert.assertFalse("Firebreak not expected - long-term sigma is zero", Calculation.isFirebreak(15.0D, 0.0D));
    }
}
