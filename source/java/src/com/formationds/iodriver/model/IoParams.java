package com.formationds.iodriver.model;

/**
 * Basic QoS parameters.
 */
public final class IoParams
{
    /**
     * Constructor.
     * 
     * @param assured Guaranteed IOPS available. Must be >= 0.
     * @param throttle Maximum IOPS allowed. Must be >= {@code assured}.
     */
    public IoParams(int assured, int throttle)
    {
        if (assured < 0)
        {
            throw new IllegalArgumentException("assured must be >= 0. assured: " + assured);
        }
        if (throttle < assured)
        {
            throw new IllegalArgumentException("throttle must be > assured. assured: "
                                               + assured + ", throttle: " + throttle);
        }

        _assured = assured;
        _throttle = throttle;
    }

    /**
     * Get the guaranteed available IOPS.
     * 
     * @return The current property value.
     */
    public int getAssured()
    {
        return _assured;
    }

    /**
     * Get the maximum allowed IOPS.
     * 
     * @return The current property value.
     */
    public int getThrottle()
    {
        return _throttle;
    }

    /**
     * Guaranteed available IOPS.
     */
    private final int _assured;

    /**
     * Maximum allowed IOPS.
     */
    private final int _throttle;
}
