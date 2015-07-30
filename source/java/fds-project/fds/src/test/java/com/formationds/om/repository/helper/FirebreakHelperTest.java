package com.formationds.om.repository.helper;

import com.formationds.apis.VolumeStatus;
import com.formationds.apis.XdiService;
import com.formationds.commons.events.FirebreakType;
import com.formationds.commons.model.entity.IVolumeDatapoint;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.om.helper.SingletonAmAPI;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.helper.SingletonConfiguration;
import com.formationds.util.Configuration;
import com.formationds.util.thrift.ConfigurationApi;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Test;

import java.time.Instant;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.EnumMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

/**
 * FirebreakHelper Tester.
 */
public class FirebreakHelperTest {

    /**
     * @return a copy of the test data set with timestamps set offset from 24 hours ago
     */
    public static VolumeDatapoint[][] getTestDataSet() {
        return getTestDataSet( Instant.now().toEpochMilli() - TimeUnit.HOURS.toMillis( 24 ), 7200 );
    }

    /**
     * @param ts       the starting timestamp
     * @param tsoffset the timestamp offset for each collection of data.
     *
     * @return a copy of the test data set with timestamps offset from the specified starting time.
     */
    public static VolumeDatapoint[][] getTestDataSet( long ts, int tsoffset ) {
        VolumeDatapoint[][] c = new VolumeDatapoint[testdps.length][];
        for ( int i = 0; i < testdps.length; i++ ) {
            c[i] = testdps[i].clone();
            for ( int j = 0; j < c[i].length; j++ ) {
                VolumeDatapoint dp = c[i][j];
                dp.setTimestamp( ts + (i * tsoffset) );
            }
        }
        return c;
    }

    // set of datapoints to generate events.  These were captured from real data.  Note
    // that the timestamp values are overwritten when processed to set them to current time
    // minus an offset;
    // NOTE: this dataset is currently used by other tests (ie FirebreakHelperTest).  If it changes, those tests may need to be updated.
    private static final VolumeDatapoint[][] testdps =
        new VolumeDatapoint[][]
            {
                {
                    new VolumeDatapoint( 1416588552440L, "3", "u1-ov1", "Puts", 11.0D ),
                    new VolumeDatapoint( 1416588552440L, "3", "u1-ov1", "Gets", 0.0D ),
                    new VolumeDatapoint( 1416588552440L, "3", "u1-ov1", "Queue Full", 0.0D ),
                    new VolumeDatapoint( 1416588552440L, "3", "u1-ov1", "Percent of SSD Accesses", 0.0D ),
                    new VolumeDatapoint( 1416588552440L, "3", "u1-ov1", "Logical Bytes", 1690657.0D ),
                    new VolumeDatapoint( 1416588552440L, "3", "u1-ov1", "Physical Bytes", 1690657.0D ),
                    new VolumeDatapoint( 1416588552440L, "3", "u1-ov1", "Metadata Bytes", 179928.0D ),
                    new VolumeDatapoint( 1416588552440L, "3", "u1-ov1", "Blobs", 2499.0D ),
                    new VolumeDatapoint( 1416588552440L, "3", "u1-ov1", "Objects", 2499.0D ),
                    new VolumeDatapoint( 1416588552440L, "3", "u1-ov1", "Ave Blob Size", 676.5334133653462D ),
                    new VolumeDatapoint( 1416588552440L, "3", "u1-ov1", "Ave Objects per Blob", 1.0D ),
                    // Capacity Firebreak
                    new VolumeDatapoint( 1416588552440L, "3", "u1-ov1", "Short Term Capacity Sigma",
                                         16.43837427210252D ),
                    new VolumeDatapoint( 1416588552440L, "3", "u1-ov1", "Long Term Capacity Sigma", 4.0D ),
                    new VolumeDatapoint( 1416588552440L, "3", "u1-ov1", "Short Term Capacity WMA",
                                         30193.157894736843D ),
                    // performance firebreak
                    new VolumeDatapoint( 1416588552440L, "3", "u1-ov1", "Short Term Perf Sigma", 15.0D ),
                    new VolumeDatapoint( 1416588552440L, "3", "u1-ov1", "Long Term Perf Sigma", 3.0D ),
                    new VolumeDatapoint( 1416588552440L, "3", "u1-ov1", "Short Term Perf WMA", 59.0D ),
                }, {
                       new VolumeDatapoint( 1416588552455L, "3", "u1-ov1", "Puts", 12.0D ),
                       new VolumeDatapoint( 1416588552455L, "3", "u1-ov1", "Gets", 0.0D ),
                       new VolumeDatapoint( 1416588552455L, "3", "u1-ov1", "Queue Full", 0.0D ),
                       new VolumeDatapoint( 1416588552455L, "3", "u1-ov1", "Percent of SSD Accesses", 0.0D ),
                       new VolumeDatapoint( 1416588552455L, "3", "u1-ov1", "Logical Bytes", 1696802.0D ),
                       new VolumeDatapoint( 1416588552455L, "3", "u1-ov1", "Physical Bytes", 1696802.0D ),
                       new VolumeDatapoint( 1416588552455L, "3", "u1-ov1", "Metadata Bytes", 180792.0D ),
                       new VolumeDatapoint( 1416588552455L, "3", "u1-ov1", "Blobs", 2511.0D ),
                       new VolumeDatapoint( 1416588552455L, "3", "u1-ov1", "Objects", 2511.0D ),
                       new VolumeDatapoint( 1416588552455L, "3", "u1-ov1", "Ave Blob Size", 675.747510951812D ),
                       new VolumeDatapoint( 1416588552455L, "3", "u1-ov1", "Ave Objects per Blob", 1.0D ),
                       new VolumeDatapoint( 1416588552455L, "3", "u1-ov1", "Short Term Capacity Sigma",
                                            16.43837427210252D ),
                       new VolumeDatapoint( 1416588552455L, "3", "u1-ov1", "Long Term Capacity Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552455L, "3", "u1-ov1", "Short Term Capacity WMA",
                                            30193.157894736843D ),
                       new VolumeDatapoint( 1416588552455L, "3", "u1-ov1", "Short Term Perf Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552455L, "3", "u1-ov1", "Long Term Perf Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552455L, "3", "u1-ov1", "Short Term Perf WMA", 59.0D ),
                }, {
                       new VolumeDatapoint( 1416588552456L, "5", "u2-ov1", "Puts", 11.0D ),
                       new VolumeDatapoint( 1416588552456L, "5", "u2-ov1", "Gets", 0.0D ),
                       new VolumeDatapoint( 1416588552457L, "5", "u2-ov1", "Queue Full", 0.0D ),
                       new VolumeDatapoint( 1416588552457L, "5", "u2-ov1", "Percent of SSD Accesses", 0.0D ),
                       new VolumeDatapoint( 1416588552457L, "5", "u2-ov1", "Logical Bytes", 1690222.0D ),
                       new VolumeDatapoint( 1416588552457L, "5", "u2-ov1", "Physical Bytes", 1690222.0D ),
                       new VolumeDatapoint( 1416588552457L, "5", "u2-ov1", "Metadata Bytes", 179496.0D ),
                       new VolumeDatapoint( 1416588552457L, "5", "u2-ov1", "Blobs", 2493.0D ),
                       new VolumeDatapoint( 1416588552457L, "5", "u2-ov1", "Objects", 2493.0D ),
                       new VolumeDatapoint( 1416588552458L, "5", "u2-ov1", "Ave Blob Size", 677.9871640593663D ),
                       new VolumeDatapoint( 1416588552458L, "5", "u2-ov1", "Ave Objects per Blob", 1.0D ),
                       new VolumeDatapoint( 1416588552458L, "5", "u2-ov1", "Short Term Capacity Sigma",
                                            7.705487940048173D ),
                       new VolumeDatapoint( 1416588552458L, "5", "u2-ov1", "Long Term Capacity Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552459L, "5", "u2-ov1", "Short Term Capacity WMA",
                                            30194.894736842107D ),
                       new VolumeDatapoint( 1416588552459L, "5", "u2-ov1", "Short Term Perf Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552459L, "5", "u2-ov1", "Long Term Perf Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552459L, "5", "u2-ov1", "Short Term Perf WMA", 59.0D ),
                }, {
                       new VolumeDatapoint( 1416588552459L, "5", "u2-ov1", "Puts", 12.0D ),
                       new VolumeDatapoint( 1416588552459L, "5", "u2-ov1", "Gets", 0.0D ),
                       new VolumeDatapoint( 1416588552459L, "5", "u2-ov1", "Queue Full", 0.0D ),
                       new VolumeDatapoint( 1416588552459L, "5", "u2-ov1", "Percent of SSD Accesses", 0.0D ),
                       new VolumeDatapoint( 1416588552459L, "5", "u2-ov1", "Logical Bytes", 1696369.0D ),
                       new VolumeDatapoint( 1416588552459L, "5", "u2-ov1", "Physical Bytes", 1696369.0D ),
                       new VolumeDatapoint( 1416588552460L, "5", "u2-ov1", "Metadata Bytes", 180360.0D ),
                       new VolumeDatapoint( 1416588552460L, "5", "u2-ov1", "Blobs", 2505.0D ),
                       new VolumeDatapoint( 1416588552460L, "5", "u2-ov1", "Objects", 2505.0D ),
                       new VolumeDatapoint( 1416588552460L, "5", "u2-ov1", "Ave Blob Size", 677.1932135728543D ),
                       new VolumeDatapoint( 1416588552460L, "5", "u2-ov1", "Ave Objects per Blob", 1.0D ),
                       new VolumeDatapoint( 1416588552460L, "5", "u2-ov1", "Short Term Capacity Sigma",
                                            7.705487940048173D ),
                       new VolumeDatapoint( 1416588552460L, "5", "u2-ov1", "Long Term Capacity Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552461L, "5", "u2-ov1", "Short Term Capacity WMA",
                                            30194.894736842107D ),
                       new VolumeDatapoint( 1416588552461L, "5", "u2-ov1", "Short Term Perf Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552461L, "5", "u2-ov1", "Long Term Perf Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552461L, "5", "u2-ov1", "Short Term Perf WMA", 59.0D )
                }, {
                       new VolumeDatapoint( 1416588552657L, "3", "u1-ov1", "Puts", 12.0D ),
                       new VolumeDatapoint( 1416588552657L, "3", "u1-ov1", "Gets", 0.0D ),
                       new VolumeDatapoint( 1416588552657L, "3", "u1-ov1", "Queue Full", 0.0D ),
                       new VolumeDatapoint( 1416588552657L, "3", "u1-ov1", "Percent of SSD Accesses", 0.0D ),
                       new VolumeDatapoint( 1416588552657L, "3", "u1-ov1", "Logical Bytes", 1702939.0D ),
                       new VolumeDatapoint( 1416588552658L, "3", "u1-ov1", "Physical Bytes", 1702939.0D ),
                       new VolumeDatapoint( 1416588552658L, "3", "u1-ov1", "Metadata Bytes", 181656.0D ),
                       new VolumeDatapoint( 1416588552658L, "3", "u1-ov1", "Blobs", 2523.0D ),
                       new VolumeDatapoint( 1416588552658L, "3", "u1-ov1", "Objects", 2523.0D ),
                       new VolumeDatapoint( 1416588552658L, "3", "u1-ov1", "Ave Blob Size", 674.9659135949266D ),
                       new VolumeDatapoint( 1416588552658L, "3", "u1-ov1", "Ave Objects per Blob", 1.0D ),
                       new VolumeDatapoint( 1416588552658L, "3", "u1-ov1", "Short Term Capacity Sigma",
                                            16.43837427210252D ),
                       new VolumeDatapoint( 1416588552658L, "3", "u1-ov1", "Long Term Capacity Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552658L, "3", "u1-ov1", "Short Term Capacity WMA",
                                            30193.157894736843D ),
                       new VolumeDatapoint( 1416588552658L, "3", "u1-ov1", "Short Term Perf Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552658L, "3", "u1-ov1", "Long Term Perf Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552659L, "3", "u1-ov1", "Short Term Perf WMA", 59.0D ),
                }, {

                       new VolumeDatapoint( 1416588552659L, "3", "u1-ov1", "Puts", 12.0D ),
                       new VolumeDatapoint( 1416588552659L, "3", "u1-ov1", "Gets", 0.0D ),
                       new VolumeDatapoint( 1416588552659L, "3", "u1-ov1", "Queue Full", 0.0D ),
                       new VolumeDatapoint( 1416588552659L, "3", "u1-ov1", "Percent of SSD Accesses", 0.0D ),
                       new VolumeDatapoint( 1416588552659L, "3", "u1-ov1", "Logical Bytes", 1709089.0D ),
                       new VolumeDatapoint( 1416588552659L, "3", "u1-ov1", "Physical Bytes", 1709089.0D ),
                       new VolumeDatapoint( 1416588552659L, "3", "u1-ov1", "Metadata Bytes", 182520.0D ),
                       new VolumeDatapoint( 1416588552659L, "3", "u1-ov1", "Blobs", 2535.0D ),
                       new VolumeDatapoint( 1416588552659L, "3", "u1-ov1", "Objects", 2535.0D ),
                       new VolumeDatapoint( 1416588552659L, "3", "u1-ov1", "Ave Blob Size", 674.1968441814596D ),
                       new VolumeDatapoint( 1416588552660L, "3", "u1-ov1", "Ave Objects per Blob", 1.0D ),
                       new VolumeDatapoint( 1416588552660L, "3", "u1-ov1", "Short Term Capacity Sigma",
                                            16.43837427210252D ),
                       new VolumeDatapoint( 1416588552660L, "3", "u1-ov1", "Long Term Capacity Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552660L, "3", "u1-ov1", "Short Term Capacity WMA",
                                            30193.157894736843D ),
                       new VolumeDatapoint( 1416588552660L, "3", "u1-ov1", "Short Term Perf Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552660L, "3", "u1-ov1", "Long Term Perf Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552660L, "3", "u1-ov1", "Short Term Perf WMA", 59.0D ),
                }, {

                       new VolumeDatapoint( 1416588552660L, "5", "u2-ov1", "Puts", 12.0D ),
                       new VolumeDatapoint( 1416588552660L, "5", "u2-ov1", "Gets", 0.0D ),
                       new VolumeDatapoint( 1416588552661L, "5", "u2-ov1", "Queue Full", 0.0D ),
                       new VolumeDatapoint( 1416588552661L, "5", "u2-ov1", "Percent of SSD Accesses", 0.0D ),
                       new VolumeDatapoint( 1416588552661L, "5", "u2-ov1", "Logical Bytes", 1702516.0D ),
                       new VolumeDatapoint( 1416588552661L, "5", "u2-ov1", "Physical Bytes", 1702516.0D ),
                       new VolumeDatapoint( 1416588552661L, "5", "u2-ov1", "Metadata Bytes", 181224.0D ),
                       new VolumeDatapoint( 1416588552661L, "5", "u2-ov1", "Blobs", 2517.0D ),
                       new VolumeDatapoint( 1416588552661L, "5", "u2-ov1", "Objects", 2517.0D ),
                       new VolumeDatapoint( 1416588552661L, "5", "u2-ov1", "Ave Blob Size", 676.4068335319826D ),
                       new VolumeDatapoint( 1416588552661L, "5", "u2-ov1", "Ave Objects per Blob", 1.0D ),
                       new VolumeDatapoint( 1416588552661L, "5", "u2-ov1", "Short Term Capacity Sigma",
                                            7.705487940048173D ),
                       new VolumeDatapoint( 1416588552662L, "5", "u2-ov1", "Long Term Capacity Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552662L, "5", "u2-ov1", "Short Term Capacity WMA",
                                            30194.894736842107D ),
                       new VolumeDatapoint( 1416588552662L, "5", "u2-ov1", "Short Term Perf Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552662L, "5", "u2-ov1", "Long Term Perf Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552662L, "5", "u2-ov1", "Short Term Perf WMA", 59.0D ),
                }, {
                       new VolumeDatapoint( 1416588552662L, "5", "u2-ov1", "Puts", 12.0D ),
                       new VolumeDatapoint( 1416588552662L, "5", "u2-ov1", "Gets", 0.0D ),
                       new VolumeDatapoint( 1416588552662L, "5", "u2-ov1", "Queue Full", 0.0D ),
                       new VolumeDatapoint( 1416588552662L, "5", "u2-ov1", "Percent of SSD Accesses", 0.0D ),
                       new VolumeDatapoint( 1416588552662L, "5", "u2-ov1", "Logical Bytes", 1708664.0D ),
                       new VolumeDatapoint( 1416588552662L, "5", "u2-ov1", "Physical Bytes", 1708664.0D ),
                       new VolumeDatapoint( 1416588552663L, "5", "u2-ov1", "Metadata Bytes", 182088.0D ),
                       new VolumeDatapoint( 1416588552663L, "5", "u2-ov1", "Blobs", 2529.0D ),
                       new VolumeDatapoint( 1416588552663L, "5", "u2-ov1", "Objects", 2529.0D ),
                       new VolumeDatapoint( 1416588552663L, "5", "u2-ov1", "Ave Blob Size", 675.628311585607D ),
                       new VolumeDatapoint( 1416588552663L, "5", "u2-ov1", "Ave Objects per Blob", 1.0D ),
                       new VolumeDatapoint( 1416588552663L, "5", "u2-ov1", "Short Term Capacity Sigma",
                                            7.705487940048173D ),
                       new VolumeDatapoint( 1416588552663L, "5", "u2-ov1", "Long Term Capacity Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552663L, "5", "u2-ov1", "Short Term Capacity WMA",
                                            30194.894736842107D ),
                       // perf
                       new VolumeDatapoint( 1416588552663L, "5", "u2-ov1", "Short Term Perf Sigma", 30.0D ),
                       new VolumeDatapoint( 1416588552663L, "5", "u2-ov1", "Long Term Perf Sigma", 3.0D ),
                       new VolumeDatapoint( 1416588552663L, "5", "u2-ov1", "Short Term Perf WMA", 59.0D ),
                }, {
                       new VolumeDatapoint( 1416588552709L, "3", "u1-ov1", "Puts", 12.0D ),
                       new VolumeDatapoint( 1416588552709L, "3", "u1-ov1", "Gets", 0.0D ),
                       new VolumeDatapoint( 1416588552709L, "3", "u1-ov1", "Queue Full", 0.0D ),
                       new VolumeDatapoint( 1416588552709L, "3", "u1-ov1", "Percent of SSD Accesses", 0.0D ),
                       new VolumeDatapoint( 1416588552709L, "3", "u1-ov1", "Logical Bytes", 1715231.0D ),
                       new VolumeDatapoint( 1416588552709L, "3", "u1-ov1", "Physical Bytes", 1715231.0D ),
                       new VolumeDatapoint( 1416588552709L, "3", "u1-ov1", "Metadata Bytes", 183384.0D ),
                       new VolumeDatapoint( 1416588552710L, "3", "u1-ov1", "Blobs", 2547.0D ),
                       new VolumeDatapoint( 1416588552710L, "3", "u1-ov1", "Objects", 2547.0D ),
                       new VolumeDatapoint( 1416588552710L, "3", "u1-ov1", "Ave Blob Size", 673.4318806438948D ),
                       new VolumeDatapoint( 1416588552710L, "3", "u1-ov1", "Ave Objects per Blob", 1.0D ),
                       new VolumeDatapoint( 1416588552710L, "3", "u1-ov1", "Short Term Capacity Sigma",
                                            16.43837427210252D ),
                       new VolumeDatapoint( 1416588552710L, "3", "u1-ov1", "Long Term Capacity Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552710L, "3", "u1-ov1", "Short Term Capacity WMA",
                                            30193.157894736843D ),
                       new VolumeDatapoint( 1416588552710L, "3", "u1-ov1", "Short Term Perf Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552711L, "3", "u1-ov1", "Long Term Perf Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552711L, "3", "u1-ov1", "Short Term Perf WMA", 59.0D ),
                }, {
                       new VolumeDatapoint( 1416588552711L, "3", "u1-ov1", "Puts", 11.0D ),
                       new VolumeDatapoint( 1416588552711L, "3", "u1-ov1", "Gets", 0.0D ),
                       new VolumeDatapoint( 1416588552711L, "3", "u1-ov1", "Queue Full", 0.0D ),
                       new VolumeDatapoint( 1416588552711L, "3", "u1-ov1", "Percent of SSD Accesses", 0.0D ),
                       new VolumeDatapoint( 1416588552711L, "3", "u1-ov1", "Logical Bytes", 1720866.0D ),
                       new VolumeDatapoint( 1416588552711L, "3", "u1-ov1", "Physical Bytes", 1720866.0D ),
                       new VolumeDatapoint( 1416588552712L, "3", "u1-ov1", "Metadata Bytes", 184176.0D ),
                       new VolumeDatapoint( 1416588552712L, "3", "u1-ov1", "Blobs", 2558.0D ),
                       new VolumeDatapoint( 1416588552712L, "3", "u1-ov1", "Objects", 2558.0D ),
                       new VolumeDatapoint( 1416588552712L, "3", "u1-ov1", "Ave Blob Size", 672.73885848319D ),
                       new VolumeDatapoint( 1416588552712L, "3", "u1-ov1", "Ave Objects per Blob", 1.0D ),
                       new VolumeDatapoint( 1416588552712L, "3", "u1-ov1", "Short Term Capacity Sigma",
                                            16.43837427210252D ),
                       new VolumeDatapoint( 1416588552712L, "3", "u1-ov1", "Long Term Capacity Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552712L, "3", "u1-ov1", "Short Term Capacity WMA",
                                            30193.157894736843D ),
                       new VolumeDatapoint( 1416588552712L, "3", "u1-ov1", "Short Term Perf Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552713L, "3", "u1-ov1", "Long Term Perf Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552713L, "3", "u1-ov1", "Short Term Perf WMA", 59.0D ),
                }, {
                       new VolumeDatapoint( 1416588552713L, "5", "u2-ov1", "Puts", 12.0D ),
                       new VolumeDatapoint( 1416588552713L, "5", "u2-ov1", "Gets", 0.0D ),
                       new VolumeDatapoint( 1416588552713L, "5", "u2-ov1", "Queue Full", 0.0D ),
                       new VolumeDatapoint( 1416588552713L, "5", "u2-ov1", "Percent of SSD Accesses", 0.0D ),
                       new VolumeDatapoint( 1416588552713L, "5", "u2-ov1", "Logical Bytes", 1714807.0D ),
                       new VolumeDatapoint( 1416588552713L, "5", "u2-ov1", "Physical Bytes", 1714807.0D ),
                       new VolumeDatapoint( 1416588552713L, "5", "u2-ov1", "Metadata Bytes", 182952.0D ),
                       new VolumeDatapoint( 1416588552714L, "5", "u2-ov1", "Blobs", 2541.0D ),
                       new VolumeDatapoint( 1416588552714L, "5", "u2-ov1", "Objects", 2541.0D ),
                       new VolumeDatapoint( 1416588552714L, "5", "u2-ov1", "Ave Blob Size", 674.8551751279024D ),
                       new VolumeDatapoint( 1416588552714L, "5", "u2-ov1", "Ave Objects per Blob", 1.0D ),
                       new VolumeDatapoint( 1416588552714L, "5", "u2-ov1", "Short Term Capacity Sigma",
                                            7.705487940048173D ),
                       new VolumeDatapoint( 1416588552714L, "5", "u2-ov1", "Long Term Capacity Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552714L, "5", "u2-ov1", "Short Term Capacity WMA",
                                            30194.894736842107D ),
                       new VolumeDatapoint( 1416588552715L, "5", "u2-ov1", "Short Term Perf Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552715L, "5", "u2-ov1", "Long Term Perf Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552715L, "5", "u2-ov1", "Short Term Perf WMA", 59.0D ),
                }, {
                       new VolumeDatapoint( 1416588552715L, "5", "u2-ov1", "Puts", 11.0D ),
                       new VolumeDatapoint( 1416588552715L, "5", "u2-ov1", "Gets", 0.0D ),
                       new VolumeDatapoint( 1416588552715L, "5", "u2-ov1", "Queue Full", 0.0D ),
                       new VolumeDatapoint( 1416588552715L, "5", "u2-ov1", "Percent of SSD Accesses", 0.0D ),
                       new VolumeDatapoint( 1416588552715L, "5", "u2-ov1", "Logical Bytes", 1720450.0D ),
                       new VolumeDatapoint( 1416588552715L, "5", "u2-ov1", "Physical Bytes", 1720450.0D ),
                       new VolumeDatapoint( 1416588552716L, "5", "u2-ov1", "Metadata Bytes", 183744.0D ),
                       new VolumeDatapoint( 1416588552716L, "5", "u2-ov1", "Blobs", 2552.0D ),
                       new VolumeDatapoint( 1416588552716L, "5", "u2-ov1", "Objects", 2552.0D ),
                       new VolumeDatapoint( 1416588552716L, "5", "u2-ov1", "Ave Blob Size", 674.1575235109718D ),
                       new VolumeDatapoint( 1416588552716L, "5", "u2-ov1", "Ave Objects per Blob", 1.0D ),
                       // capacity fb
                       new VolumeDatapoint( 1416588552716L, "5", "u2-ov1", "Short Term Capacity Sigma",
                                            7.705487940048173D ),
                       new VolumeDatapoint( 1416588552716L, "5", "u2-ov1", "Long Term Capacity Sigma", 1.0D ),
                       new VolumeDatapoint( 1416588552716L, "5", "u2-ov1", "Short Term Capacity WMA",
                                            30194.894736842107D ),
                       new VolumeDatapoint( 1416588552716L, "5", "u2-ov1", "Short Term Perf Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552717L, "5", "u2-ov1", "Long Term Perf Sigma", 0.0D ),
                       new VolumeDatapoint( 1416588552717L, "5", "u2-ov1", "Short Term Perf WMA", 59.0D ),
                }
            };

    static final ConfigurationApi mockedConfigApi = mock( ConfigurationApi.class );
    static final Configuration    mockedConfig    = mock( Configuration.class );
    static final XdiService.Iface mockedAMService = mock( XdiService.Iface.class );

    @BeforeClass
    static public void setUpClass() throws Exception {
        SingletonConfigAPI.instance().api( mockedConfigApi );
        SingletonConfiguration.instance().setConfig( mockedConfig );
        SingletonAmAPI.instance().api( mockedAMService );
        VolumeStatus vstat = new VolumeStatus();
        vstat.setCurrentUsageInBytes( 1024 );
        when( mockedAMService.volumeStatus( "", "u1-ov1" ) ).thenReturn( vstat );
        when( mockedAMService.volumeStatus( "", "u2-ov1" ) ).thenReturn( vstat );
    }

    @Before
    public void before() throws Exception {
    }

    @After
    public void after() throws Exception {
    }

    /**
     * Method: processFirebreak(final List<VolumeDatapoint> queryResults)
     */
    @Test
    @Ignore //not implemented
    public void testProcessFirebreak() throws Exception {
        //TODO: Test goes here... 
    }

    /**
     * Method: findFirebreak(final List<VolumeDatapoint> datapoints)
     */
    @Test
    @Ignore("Test fails if InfluxDB is not running.  Works with JDO b/c ObjectDB is embedded.")
    public void testFindFirebreak() throws Exception {
        VolumeDatapoint[][] rawdata = getTestDataSet();
        List<VolumeDatapoint> all = new ArrayList<>();
        Arrays.asList( rawdata ).forEach( (( r ) -> all.addAll( Arrays.asList( r ) )) );

        FirebreakHelper fbh = new FirebreakHelper();
        Map<String, FirebreakHelper.VolumeDatapointPair> m = fbh.findFirebreak( all );

        Assert.assertEquals( 2, m.size() );
        String v1 = "3";
        String v2 = "5";
        Assert.assertNotNull( m.get( v1 ) );
        Assert.assertNotNull( m.get( v2 ) );
    }

    /**
     * Method: findFirebreakEvents(final List<VolumeDatapoint> datapoints)
     */
    @Test
    public void testFindFirebreakEvents() throws Exception {
        VolumeDatapoint[][] rawdata = getTestDataSet();
        List<VolumeDatapoint> all = new ArrayList<>();
        Arrays.asList( rawdata ).forEach( (( r ) -> all.addAll( Arrays.asList( r ) )) );

        FirebreakHelper fbh = new FirebreakHelper();
        Map<Long, EnumMap<FirebreakType, FirebreakHelper.VolumeDatapointPair>> m = fbh.findFirebreakEvents( all );

        //
        Assert.assertEquals( 2, m.size() );
        Long v1 = 3L;
        Long v2 = 5L;
        Assert.assertNotNull( m.get( v1 ) );
        Assert.assertNotNull( m.get( v2 ) );

        Assert.assertEquals( 2, m.get( v1 ).size() );
        Assert.assertEquals( 2, m.get( v2 ).size() );
    }

    /**
     * Method: extractPairs(List<VolumeDatapoint> datapoints)
     */
    @Test
    public void testExtractPairs() throws Exception {
        VolumeDatapoint[][] rawdata = getTestDataSet();
        List<VolumeDatapoint[]> rawdatal = Arrays.asList( rawdata );
        rawdatal.forEach( ( r ) -> {
            // Assumptions here that the raw data from EventManagerTest only contains data for a
            // single volume at a time.  That is not necessarily true of data from the stats stream
            List<FirebreakHelper.VolumeDatapointPair> vdps = new FirebreakHelper().extractPairs( Arrays.asList( r ) );
            Assert.assertTrue( "test data has 2 firebreak datapoint pairs each", vdps.size() == 2 );
            Assert.assertEquals( FirebreakType.CAPACITY, vdps.get( 0 ).getFirebreakType() );
            Assert.assertEquals( FirebreakType.PERFORMANCE, vdps.get( 1 ).getFirebreakType() );
        } );

        // this time we combine all of the data across timestamps and volumes.
        List<VolumeDatapoint> all = new ArrayList<>();
        rawdatal.forEach( ( r ) -> Arrays.asList( r ).forEach( all::add ) );
        List<FirebreakHelper.VolumeDatapointPair> vdps = new FirebreakHelper().extractPairs( all );

        // data set has 12 entries each with 2 pairs so we expect a total of 24 pairs back
        Assert.assertTrue( vdps.size() == (2 * rawdatal.size()) );

        // this third tests is a negative test to make sure we handle
        // 0 entries
        // odd number of entries
        // non-firebreak entries
        List<VolumeDatapoint> neg = Arrays.asList( rawdatal.get( 0 ) );
        List<VolumeDatapoint> missingSTC = neg.stream()
                                              .filter((v) -> !Metrics.STC_SIGMA.equals(Metrics.lookup( v.getKey() )))
                                              .collect(Collectors.toList());

        // currently the implementation will not return anything for unpaired sigmas.  It does not
        // report any errors either.
        List<FirebreakHelper.VolumeDatapointPair> missingSTCPairs = new FirebreakHelper().extractPairs( missingSTC );
        Assert.assertTrue( missingSTCPairs.size() == 1 );
        Assert.assertTrue( missingSTCPairs.get( 0 ).getFirebreakType().equals( FirebreakType.PERFORMANCE ) );

        List<FirebreakHelper.VolumeDatapointPair> empty = new FirebreakHelper().extractPairs( new ArrayList<>() );
        Assert.assertTrue( empty.isEmpty() );
    }

    /**
     * Method: extractFirebreakDatapoints(List<VolumeDatapoint> datapoints)
     */
    @Test
    public void testExtractFirebreakDatapoints() throws Exception {
        VolumeDatapoint[][] rawdata = getTestDataSet();
        List<VolumeDatapoint[]> rawdatal = Arrays.asList( rawdata );
        rawdatal.forEach( ( r ) -> {
            // Assumptions here that the raw data from EventManagerTest only contains data for a
            // single volume at a time.  That is not necessarily true of data from the stats stream
            List<IVolumeDatapoint> vdps = new FirebreakHelper().extractFirebreakDatapoints( Arrays.asList( r ) );
            Assert.assertTrue( "test data has 4 firebreak points each", vdps.size() == 4 );

            Assert.assertTrue( vdps.get( 0 ).getKey().equals( Metrics.STC_SIGMA.key() ) );
            Assert.assertTrue( vdps.get( 1 ).getKey().equals( Metrics.LTC_SIGMA.key() ) );
            Assert.assertTrue( vdps.get( 2 ).getKey().equals( Metrics.STP_SIGMA.key() ) );
            Assert.assertTrue( vdps.get( 3 ).getKey().equals( Metrics.LTP_SIGMA.key() ) );
        } );

        // this one first strips the firebreak metrics from the lists and then ensures that
        // extractFirebreakDatapoints returns an empty list.
        rawdatal.forEach( ( r ) -> {
            List<VolumeDatapoint> stripped = Arrays.asList( r )
                                                   .stream()
                                                   .filter( ( v ) -> !v.getKey().contains( "Sigma" ) )
                                                   .collect( Collectors.toList() );

            List<IVolumeDatapoint> fbdps = new FirebreakHelper().extractFirebreakDatapoints( stripped );
            Assert.assertTrue( fbdps.isEmpty() );
        } );
    }

    /**
     * Method: isFirebreak(VolumeDatapointPair p)
     */
    @Test
    public void testIsFirebreak() throws Exception {
        FirebreakHelper fbh = new FirebreakHelper();
        Long ts = Instant.now().toEpochMilli();
        FirebreakHelper.VolumeDatapointPair cap1 = newVDPair( ts, FirebreakType.CAPACITY, 0.0D, 0.0D );
        FirebreakHelper.VolumeDatapointPair cap2 = newVDPair( ts, FirebreakType.CAPACITY, 0.0D, 10.0D );
        FirebreakHelper.VolumeDatapointPair cap3 = newVDPair( ts, FirebreakType.CAPACITY, 10.0D, 0.0D );
        FirebreakHelper.VolumeDatapointPair cap4 = newVDPair( ts, FirebreakType.CAPACITY, 12.0D, 4.0D );
        FirebreakHelper.VolumeDatapointPair cap5 = newVDPair( ts, FirebreakType.CAPACITY, 12.0D, 3.0D );
        Assert.assertFalse( fbh.isFirebreak( cap1 ) );
        Assert.assertFalse( fbh.isFirebreak( cap2 ) );
        Assert.assertFalse( fbh.isFirebreak( cap3 ) );
        Assert.assertFalse( fbh.isFirebreak( cap4 ) );
        Assert.assertTrue( fbh.isFirebreak( cap5 ) );

        FirebreakHelper.VolumeDatapointPair p1 = newVDPair( ts, FirebreakType.PERFORMANCE, 0.0D, 0.0D );
        FirebreakHelper.VolumeDatapointPair p2 = newVDPair( ts, FirebreakType.PERFORMANCE, 0.0D, 100.0D );
        FirebreakHelper.VolumeDatapointPair p3 = newVDPair( ts, FirebreakType.PERFORMANCE, 100.0D, 0.0D );
        FirebreakHelper.VolumeDatapointPair p4 = newVDPair( ts, FirebreakType.PERFORMANCE, 120.0D, 40.0D );
        FirebreakHelper.VolumeDatapointPair p5 = newVDPair( ts, FirebreakType.PERFORMANCE, 120.0D, 30.0D );
        Assert.assertFalse( fbh.isFirebreak( p1 ) );
        Assert.assertFalse( fbh.isFirebreak( p2 ) );
        Assert.assertFalse( fbh.isFirebreak( p3 ) );
        Assert.assertFalse( fbh.isFirebreak( p4 ) );
        Assert.assertTrue( fbh.isFirebreak( p5 ) );
    }

    private FirebreakHelper.VolumeDatapointPair newVDPair( Long ts, FirebreakType t, double st, double lt ) {
        VolumeDatapoint stp = new VolumeDatapoint( ts, "1", "v1",
                                                   (FirebreakType.CAPACITY.equals( t ) ? Metrics.STC_SIGMA.key()
                                                                                       : Metrics.STP_SIGMA.key()),
                                                   st );
        VolumeDatapoint ltp = new VolumeDatapoint( ts, "1", "v1",
                                                   (FirebreakType.CAPACITY.equals( t ) ? Metrics.LTC_SIGMA.key()
                                                                                       : Metrics.LTP_SIGMA.key()),
                                                   lt );
        return new FirebreakHelper.VolumeDatapointPair( stp, ltp );
    }
}

