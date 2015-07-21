
class Metric(object):
    '''
    Created on Jun 29, 2015

    just a string collection of valid metric types 
    
    @author: nate
    '''
    GETS = ("GETS", "Gets")
    PUTS = ("PUTS", "Puts")
    #   gets from SSD and cache
    SSD_GETS = ( "SSD_GETS", "SSD Gets" )
    
    QFULL = ("QFULL", "Queue Full" )
    MBYTES = ( "MBYTES", "Metadata Bytes" )
    BLOBS = ( "BLOBS", "Blobs" )
    OBJECTS = ( "OBJECTS", "Objects" )
    ABS = ( "ABS", "Ave Blob Size" )
    AOPB = ( "AOPB", "Ave Objects per Blob" )
    LBYTES = ( "LBYTES", "Logical Bytes" )
    PBYTES = ( "PBYTES", "Physical Bytes" )
#    firebreak metrics
    STC_SIGMA = ( "STC_SIGMA", "Short Term Capacity Sigma" )
    LTC_SIGMA = ( "LTC_SIGMA", "Long Term Capacity Sigma" )
    STP_SIGMA = ( "STP_SIGMA", "Short Term Perf Sigma" )
    LTP_SIGMA = ( "LTP_SIGMA", "Long Term Perf Sigma" )
#   Performance IOPS
    STC_WMA = ( "STC_WMA", "Short Term Capacity WMA" )
    LTC_WMA = ( "LTC_WMA", "Long Term Capacity WMA" )
    STP_WMA = ( "STP_WMA", "Short Term Perf WMA" )
    LTP_WMA = ( "LTP_WMA", "Long Term Perf WMA" )
    
    DICT = dict()
    
    DICT[GETS[0]] = GETS
    DICT[PUTS[0]] = PUTS
    DICT[SSD_GETS[0]] = SSD_GETS
    DICT[QFULL[0]] = QFULL
    DICT[MBYTES[0]] = MBYTES
    DICT[BLOBS[0]] = BLOBS
    DICT[OBJECTS[0]] = OBJECTS
    DICT[ABS[0]] = ABS
    DICT[AOPB[0]] = AOPB
    DICT[LBYTES[0]] = LBYTES
    DICT[PBYTES[0]] = PBYTES
    DICT[STC_SIGMA[0]] = STC_SIGMA
    DICT[LTC_SIGMA[0]] = LTC_SIGMA
    DICT[STP_SIGMA[0]] = STP_SIGMA
    DICT[LTP_SIGMA[0]] = LTP_SIGMA
    DICT[STC_WMA[0]] = STC_WMA
    DICT[LTC_WMA[0]] = LTC_WMA
    DICT[STP_WMA[0]] = STP_WMA
    DICT[LTP_WMA[0]] = LTP_WMA