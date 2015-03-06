/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

namespace cpp FDS_ProtocolInterface
namespace java com.formationds.streaming

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
   1: string                 volume_name;
   2: i64                    timestamp;
   3: list<DataPointPair>    meta_list;
}

// AM/XDI serves this
service Streaming {
   void publishMetaStream(1:i32 registration_id, 2:list<volumeDataPoints> data_points)
}
