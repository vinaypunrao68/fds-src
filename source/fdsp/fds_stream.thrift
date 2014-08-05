/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

namespace cpp FDS_ProtocolInterface


/*
 * Register stream with OM
 */
struct registerStreamMsg {
   1: string            ip_address;    /* IP address of the machine to send data */
   2: int32             port;          /* port number to send data */
   3: list<i64>         volid_list;    /* list of volume ids */
   4: i32               sample_freq;   /* Frequency in seconds for sampling data, currently valid: 1 min or 1 hour */
   5: i32               duration;      /* samples will be sent every specified duration in seconds */
}

/*
 * Response from OM to register stream message
 * error code will be in the header 
 */
struct registerStreamRspMsg {
}

/*
 * De-register stream with OM
 */
struct deregisterStreamMsg {
   1: list<i64>        volid_list;  /* list of volume ids */
}


/*
 * Data point for streamed metadata
 * Example: key = capacity, value = 100000
 * we will document interpretation of data (e.g. in bytes)
 */
struct DataPointPair {
   1: string     key;
   2: double     value;
}

/**
 * Volume's metadata for a particular sampled time interval
 */
struct volumeDataPoints {
   1: i64                    volume_id;
   2: i64                    timestamp;
   3: list<DataPointPair>    meta_list;
}

/*
 * Per-volume meta stream pushed to AM
 */
struct metaStreamMsg {
   1: list<volumeDataPoints>  volmeta_list;
   2: string                  ip_address;    /* IP address of the machine to send data, that was provided by registerStreamMsg  */
   4: int32                   port;          /* port number to send data, that was provided by registerStreamMsg  */
}
