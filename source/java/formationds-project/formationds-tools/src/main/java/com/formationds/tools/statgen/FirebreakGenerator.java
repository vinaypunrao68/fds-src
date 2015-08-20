/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.tools.statgen;

import com.formationds.tools.statgen.StatValueGenerators.RndSVGD;

import java.util.EnumMap;

/**
 * Generate firebreaks
 */
public class FirebreakGenerator implements StatValueGenerator<EnumMap<FirebreakType, FirebreakMetrics>> {

    private RndSVGD capRngD  = new RndSVGD( 0D, 10D );
    private RndSVGD perfRngD = new RndSVGD( 0D, 10D );

    // todo: this forces a firebreak at this frequency but given the randomness they may occur more frequently
    private int capacityFrequency    = 12 * 60;  // assuming 1 minute interval, capacityFB every 12 hours
    private int performanceFrequency = 18 * 60;

    private int cnt;

    public FirebreakGenerator() {
    }

    public FirebreakGenerator( int capacityFrequency, int performanceFrequency ) {
        this.capacityFrequency = capacityFrequency;
        this.performanceFrequency = performanceFrequency;
    }

    @Override
    public EnumMap<FirebreakType, FirebreakMetrics> generate() {

        FirebreakMetrics capacityFB;
        FirebreakMetrics performanceFB;
        if ( cnt % capacityFrequency == 0 ) {
            capacityFB = FirebreakMetrics.capacity( 24D, 4D, 0D, 0D );
        } else {
            capacityFB = FirebreakMetrics.capacity( capRngD.generate(),
                                                    capRngD.generate(),
                                                    0D,
                                                    0D );
        }

        if ( cnt % performanceFrequency == 0 ) {
            performanceFB = FirebreakMetrics.performance( 21D, 3D, 0D, 0D );
        } else {
            performanceFB = FirebreakMetrics.performance( perfRngD.generate(),
                                                          perfRngD.generate(),
                                                          0D,
                                                          0D );
        }

        EnumMap<FirebreakType, FirebreakMetrics> fbmap = new EnumMap<>( FirebreakType.class );
        fbmap.put( FirebreakType.CAPACITY, capacityFB );
        fbmap.put( FirebreakType.PERFORMANCE, performanceFB );

        cnt++;

        return fbmap;
    }
}
