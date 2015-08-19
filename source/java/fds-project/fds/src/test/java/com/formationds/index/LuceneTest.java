package com.formationds.index;

import com.formationds.apis.MediaPolicy;
import com.formationds.apis.VolumeSettings;
import com.formationds.apis.VolumeType;
import com.formationds.commons.Fds;
import com.formationds.util.Configuration;
import com.formationds.util.ServerPortFinder;
import com.formationds.xdi.AsyncAm;
import com.formationds.xdi.RealAsyncAm;
import com.formationds.xdi.XdiClientFactory;
import com.formationds.xdi.XdiConfigurationApi;
import com.google.common.collect.Lists;
import org.apache.lucene.analysis.standard.StandardAnalyzer;
import org.apache.lucene.document.*;
import org.apache.lucene.index.DirectoryReader;
import org.apache.lucene.index.IndexWriter;
import org.apache.lucene.index.IndexWriterConfig;
import org.apache.lucene.index.Term;
import org.apache.lucene.search.*;
import org.apache.lucene.store.Directory;
import org.joda.time.DateTime;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Test;

import java.util.Collections;
import java.util.List;
import java.util.Random;
import java.util.UUID;
import java.util.concurrent.TimeUnit;

@Ignore
public class LuceneTest {

    public static final List<String> KEYS = Lists.newArrayList("giraffe", "turtle", "panda", "lemur");
    public static final List<Long> VOLUMES = Lists.newArrayList(1l, 2l, 3l, 4l);
    private String volumeName;
    private String domainName;
    private String blobName;

    public static class DataPoint {
        public static final String TIMESTAMP = "TIMESTAMP";
        public static final String TIMESTAMP_SORT = "TIMESTAMP_SORT";
        public static final String VOLUME_ID = "VOLUME_ID";
        public static final String KEY = "KEY";
        public static final String VALUE = "VALUE";
        private DateTime timestamp;
        private long volumeId;
        private String key;
        private double value;

        public DataPoint(DateTime timestamp, long volumeId, String key, double value) {
            this.timestamp = timestamp;
            this.volumeId = volumeId;
            this.key = key;
            this.value = value;
        }

        public Document asDocument() {
            Document document = new Document();
            document.add(new LongField(TIMESTAMP, timestamp.getMillis(), Field.Store.YES));
            document.add(new NumericDocValuesField(TIMESTAMP_SORT, timestamp.getMillis()));
            document.add(new LongField(VOLUME_ID, volumeId, Field.Store.YES));
            document.add(new StringField(KEY, key, Field.Store.YES));
            document.add(new DoubleField(VALUE, value, Field.Store.YES));
            return document;
        }
    }

    Random random = new Random();

    @Test
    public void testIndex() throws Exception {
        new Configuration("luceneTest", new String[]{"--console"});
        Directory directory = new FdsLuceneDirectory(asyncAm, volumeName, OBJECT_SIZE);
        IndexWriterConfig conf = new IndexWriterConfig(new StandardAnalyzer());
        conf.setMaxBufferedDocs(1024 * 1024);
        IndexWriter indexWriter = new IndexWriter(directory, conf);
        int docs = 1024 * 1024;
        long then = System.currentTimeMillis();
        for (int i = 0; i < docs; i++) {
            indexWriter.addDocument(randomDocument(i));
        }
        indexWriter.commit();
        indexWriter.close();
        long elapsed = System.currentTimeMillis() - then;
        System.out.println("Indexed " + docs + " in " + elapsed + "ms");
        IndexSearcher indexSearcher = new IndexSearcher(DirectoryReader.open(directory));

        BooleanQuery q = new BooleanQuery();
        NumericRangeQuery<Long> dateClause = NumericRangeQuery.newLongRange(DataPoint.TIMESTAMP, 0l, docs / 2l, true, true);
        NumericRangeQuery<Long> leftVolumeClause = NumericRangeQuery.newLongRange(DataPoint.VOLUME_ID, 1, 2l, 2l, true, true);
        NumericRangeQuery<Long> rightVolumeClause = NumericRangeQuery.newLongRange(DataPoint.VOLUME_ID, 1, 4l, 4l, true, true);
        BooleanQuery volumeQuery = new BooleanQuery();
        volumeQuery.add(leftVolumeClause, BooleanClause.Occur.SHOULD);
        volumeQuery.add(rightVolumeClause, BooleanClause.Occur.SHOULD);
        TermQuery leftTerm = new TermQuery(new Term(DataPoint.KEY, "turtle"));
        TermQuery rightTerm = new TermQuery(new Term(DataPoint.KEY, "panda"));
        BooleanQuery keyQuery = new BooleanQuery();
        keyQuery.add(leftTerm, BooleanClause.Occur.SHOULD);
        keyQuery.add(rightTerm, BooleanClause.Occur.SHOULD);
        q.add(dateClause, BooleanClause.Occur.MUST);
        q.add(volumeQuery, BooleanClause.Occur.MUST);
        q.add(keyQuery, BooleanClause.Occur.MUST);

        SortField field = new SortField(DataPoint.TIMESTAMP_SORT, SortField.Type.LONG);
        Sort sort = new Sort(field);

        then = System.currentTimeMillis();
        TopFieldDocs results = indexSearcher.search(q, indexSearcher.getIndexReader().maxDoc(), sort);
        elapsed = System.currentTimeMillis() - then;
        System.out.println("First search in " + elapsed + "ms");

        then = System.currentTimeMillis();
        results = indexSearcher.search(q, indexSearcher.getIndexReader().maxDoc(), sort);
        elapsed = System.currentTimeMillis() - then;

        System.out.println("Second search in " + elapsed + "ms");
        System.out.println("Count: " + results.totalHits);
//        for (int i = 0; i < results.totalHits; i++) {
//            Document doc = indexSearcher.doc(results.scoreDocs[i].doc);
//            System.out.println("##########");
//            System.out.println("timestamp:" + doc.get(DataPoint.TIMESTAMP));
//            System.out.println("volume_id:" + doc.get(DataPoint.VOLUME_ID));
//            System.out.println("key:" + doc.get(DataPoint.KEY));
//            System.out.println("value:" + doc.get(DataPoint.VALUE));
//        }
    }

    public Document randomDocument(long timestamp) {
        Collections.shuffle(KEYS);
        Collections.shuffle(VOLUMES);
        return new DataPoint(
                new DateTime(timestamp), VOLUMES.get(0), KEYS.get(0), random.nextDouble()
        ).asDocument();
    }

    private static XdiConfigurationApi config;
    private static AsyncAm asyncAm;
    private static int OBJECT_SIZE = 2 * 1024 * 1024;

    @Before
    public void setUp() throws Exception {
        domainName = UUID.randomUUID().toString();
        volumeName = UUID.randomUUID().toString();
        blobName = UUID.randomUUID().toString();
        config.createVolume(domainName, volumeName, new VolumeSettings(OBJECT_SIZE, VolumeType.OBJECT, 0, 0, MediaPolicy.HDD_ONLY), 0);
    }

    @BeforeClass
    public static void staticSetUp() throws Exception {
        XdiClientFactory xdiCf = new XdiClientFactory();
        config = new XdiConfigurationApi(xdiCf.remoteOmService(Fds.getFdsHost(), 9090));
        int serverPort = new ServerPortFinder().findPort("LuceneTest", 10000);
        asyncAm = new RealAsyncAm(Fds.getFdsHost(), 8899, serverPort, 10, TimeUnit.MINUTES);
        asyncAm.start();
    }
}
