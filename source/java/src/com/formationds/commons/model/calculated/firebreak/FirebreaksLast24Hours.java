/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.calculated.firebreak;

import com.formationds.commons.model.abs.Calculated;

/**
 * @author ptinius
 */
public class FirebreaksLast24Hours
    extends Calculated {
    private static final long serialVersionUID = 4263131689905575027L;

    /*
     * number of firebreak events that have occurred within the last 24 hours.
     */

    private Integer count;

    /**
     * @param count the {@link Integer} representing the number of firebreak
     *              occurrences in the last 24 hours
     */
    public FirebreaksLast24Hours( final Integer count ) {
        this.count = count;
    }

    /**
     * @return Returns the {@link Integer} representing the number of firebreak
     * occurrences in the last 24 hours
     */
    public Integer getCount() {
        return count;
    }
}
