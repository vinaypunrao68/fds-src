/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.tools.statgen;

import java.util.List;

/**
 * Interface used by the StatGenerator to send stats.  OMConfigServiceRestClient provides
 * an implementation that sends the stats to an OM.  Other implementations might write to
 * a file, send directly to influxdb or another stats service.
 */
public interface StatWriter {
    void sendStats( List<VolumeDatapoint> vdps );
    // TODO: streaming api could be more efficient but not quite sure how to
    // stream in a REST call offhand (though i'm sure it is doable)
    // void sendStats(Stream<VolumeDatapoint> vdp);
}
