/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.calculated.performance;

import com.formationds.commons.model.abs.Calculated;

/**
 * @author ptinius
 */
public class IOPsCapacity
    extends Calculated {
    private static final long serialVersionUID = -2163031845394624386L;

    /*
     * the system wide IOPs capacity, daily average
     */

    private Double dailyAverage;

    /**
     * @param dailyAverage the {@link Double} representing the calculated daily average
     */
    public IOPsCapacity( final Double dailyAverage ) {
        this.dailyAverage = dailyAverage;
    }

    /**
     * @return Returns {@link Double} representing the daily average
     */
    public Double getDailyAverage() {
        return dailyAverage;
    }
}
