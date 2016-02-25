package com.formationds.index;

import com.formationds.nfs.IoOps;
import com.formationds.nfs.MemoryIoOps;
import org.apache.lucene.analysis.standard.StandardAnalyzer;
import org.apache.lucene.document.Document;
import org.apache.lucene.document.Field;
import org.apache.lucene.document.TextField;
import org.apache.lucene.index.DirectoryReader;
import org.apache.lucene.index.IndexWriter;
import org.apache.lucene.index.IndexWriterConfig;
import org.apache.lucene.search.IndexSearcher;
import org.apache.lucene.store.IOContext;
import org.apache.lucene.store.IndexOutput;
import org.junit.Before;
import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class FdsLuceneDirectoryTest {

    public static final int MAX_OBJECT_SIZE = 1024 * 1024;
    private FdsLuceneDirectory directory;

    @Test
    public void testListCreateRename() throws Exception {
        IndexOutput output = directory.createOutput("foo", IOContext.DEFAULT);
        output.writeString("hello");
        output.close();
        String[] result = directory.listAll();
        assertEquals(1, result.length);
        assertEquals("foo", result[0]);
        assertEquals(6, directory.fileLength("foo"));
        directory.renameFile("foo", "bar");
        result = directory.listAll();
        assertEquals(1, result.length);
        assertEquals("bar", result[0]);
        assertEquals(6, directory.fileLength("bar"));
    }

    @Test
    public void testBasic() throws Exception {
        IndexWriter indexWriter = new IndexWriter(directory, new IndexWriterConfig(new StandardAnalyzer()));
        Document document = new Document();
        document.add(new TextField("foo", "bar", Field.Store.YES));
        indexWriter.addDocument(document);
        indexWriter.commit();
        indexWriter.close();
        DirectoryReader reader = DirectoryReader.open(directory);
        IndexSearcher searcher = new IndexSearcher(reader);
        Document doc = searcher.doc(0);
        assertEquals("bar", doc.get("foo"));
    }

    @Before
    public void setUp() throws Exception {
        IoOps ops = new MemoryIoOps(MAX_OBJECT_SIZE);
        directory = new FdsLuceneDirectory(ops, "", "foo", MAX_OBJECT_SIZE);
    }
}
