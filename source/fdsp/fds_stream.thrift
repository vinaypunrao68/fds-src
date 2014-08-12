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

struct StreamingRegistrationMsg {
   1:i32 id,
   2:string url,
   3:list<string> volume_names,
   4:i32 sample_freq_seconds,
   5:i32 duration_seconds,
}

// OM serves this
service StreamingConfiguration {
   i32 registerStream(
        1:string url,
        2:string http_method,
        3:list<string> volume_names,
        4:i32 sample_freq_seconds,
        5:i32 duration_seconds),

   list<StreamingRegistrationMsg> list_registrations(1:i32 thrift_sucks),
   void deregisterStream(1:i32 registration_id),
}


// AM/XDI serves this
service Streaming {
   void publishMetaStream(1:i32 registration_id, 2:list<volumeDataPoints> data_points)
}
