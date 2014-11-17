/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.calculated.performance;

import com.formationds.commons.model.abs.Calculated;

/**
 * @author ptinius
 */
public class IOPsConsumed
    extends Calculated {
    private static final long serialVersionUID = 4237123472208940852L;

    /*
     * the system wide IOPs consumed, daily average
    */

    private Double dailyAverage;

    /**
     * default constructor
     */
    public IOPsConsumed( ) {
    }

    /**
     * @param dailyAverage the {@link Double} representing the daily average
     */
    public void setDailyAverage( final Double dailyAverage ) {
        this.dailyAverage = dailyAverage;
    }

    /**
     * @return Returns {@link Double} representing the daily average
     */
    public Double getDailyAverage() {
        return dailyAverage;
    }
}
