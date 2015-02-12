package com.formationds.iodriver.model;

public final class IoParams
{
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

    public int getAssured()
    {
        return _assured;
    }

    public int getThrottle()
    {
        return _throttle;
    }

    private final int _assured;

    private final int _throttle;
}
