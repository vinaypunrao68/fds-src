package com.formationds.iodriver.model;

import java.time.Instant;

/**
 * Statistics for an indivual volume's performance.
 */
public final class VolumeQosPerformance
{
    /**
     * Constructor.
     */
    public VolumeQosPerformance()
    {
        this(0, null, null);
    }

    /**
     * Constructor.
     * 
     * @param ops Number of I/O operations performed. Must be <= 0.
     * @param start When the I/O began. May be {@code null} if not yet started.
     * @param stop When the I/O completed. Must be {@code null} if not yet started.
     */
    public VolumeQosPerformance(int ops, Instant start, Instant stop)
    {
        if (ops < 0)
        {
            throw new IllegalArgumentException("ops of " + ops + " is not >= 0.");
        }
        if (start == null && stop != null)
        {
            throw new IllegalArgumentException("Cannot have a stop time without a start.");
        }
        else if (start != null)
        {
            if (stop != null && start.isAfter(stop))
            {
                throw new IllegalArgumentException("Cannot stop before starting: " + start + " > "
                                                   + stop);
            }
        }

        _ops = ops;
        _start = start;
        _stop = stop;
    }

    /**
     * Add a number of operations to this volume's count.
     * 
     * @param ops Number of I/O operations to add.
     * 
     * @return The total number of I/O operations after adding {@code ops}.
     */
    public int addOps(int ops)
    {
        if (ops < 0)
        {
            throw new IllegalArgumentException(ops + " ops is less than minimum of 0.");
        }
        verifyNotStopped();

        return _ops += ops;
    }

    /**
     * Perform a deep copy of this object.
     * 
     * @return A new duplicate object.
     */
    public VolumeQosPerformance copy()
    {
        return new VolumeQosPerformance(_ops, _start, _stop);
    }

    /**
     * Get the number of I/O operations performed. This must not be called until stopped.
     * 
     * @return Operations performed.
     */
    public int getOps()
    {
        verifyStopped();

        return _ops;
    }

    /**
     * Get the instant operations began. This must not be called until started.
     * 
     * @return An instant in time.
     */
    public Instant getStart()
    {
        verifyStarted();

        return _start;
    }

    /**
     * Get the instant operations ended. This must not be called until stopped.
     * 
     * @return An instant in time.
     */
    public Instant getStop()
    {
        verifyStopped();

        return _stop;
    }

    /**
     * Get whether operations have started on this volume.
     * 
     * @return The current property value.
     */
    public boolean isStarted()
    {
        return _start != null;
    }

    /**
     * Get whether operations have ended on this volume.
     * 
     * @return The current property value.
     */
    public boolean isStopped()
    {
        return _stop != null;
    }

    /**
     * Start timing operations on this value. This must not be called twice.
     * 
     * @return The instant operations started.
     */
    public Instant startNow()
    {
        verifyNotStarted();

        return _start = Instant.now();
    }

    /**
     * Stop timing operations on this value. This must not be called before starting, and must not
     * be called twice.
     * 
     * @return The instant operations ended.
     */
    public Instant stopNow()
    {
        verifyStarted();
        verifyNotStopped();

        return _stop = Instant.now();
    }

    /**
     * Raise an error if operations have started on this volume.
     */
    public void verifyNotStarted()
    {
        if (isStarted())
        {
            throw new IllegalStateException("Already started.");
        }
    }

    /**
     * Raise an error if operations have ended on this volume.
     */
    public void verifyNotStopped()
    {
        if (isStopped())
        {
            throw new IllegalStateException("Already stopped.");
        }
    }

    /**
     * Raise an error if operations have not started on this volume.
     */
    public void verifyStarted()
    {
        if (!isStarted())
        {
            throw new IllegalStateException("Not started yet.");
        }
    }

    /**
     * Raise an error if operations have not ended on this volume.
     */
    public void verifyStopped()
    {
        if (!isStopped())
        {
            throw new IllegalStateException("Not stopped yet.");
        }
    }

    /**
     * The number of I/O operations on this volume.
     */
    private int _ops;

    /**
     * The instant operations began. May be {@code null} if {@link #_stop} is {@code null}.
     */
    private Instant _start;

    /**
     * The instant operations ended. May be {@code null}.
     */
    private Instant _stop;
}
