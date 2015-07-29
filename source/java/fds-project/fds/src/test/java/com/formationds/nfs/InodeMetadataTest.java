package com.formationds.nfs;

import org.apache.lucene.document.Document;
import org.dcache.nfs.vfs.Stat;
import org.junit.Test;

import javax.security.auth.Subject;

import static org.junit.Assert.assertEquals;

public class InodeMetadataTest {
    @Test
    public void testAsDocument() throws Exception {
        InodeMetadata metadata = new InodeMetadata(Stat.Type.DIRECTORY, new Subject(), 0644, 42, 43)
                .withLink(100, "foo")
                .withLink(200, "bar");
        Document document = metadata.asDocument();
        InodeMetadata thawed = new InodeMetadata(document);
        assertEquals(metadata, thawed);
    }
}