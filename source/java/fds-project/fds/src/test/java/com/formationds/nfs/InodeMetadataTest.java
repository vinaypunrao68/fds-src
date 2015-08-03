package com.formationds.nfs;

import org.dcache.nfs.vfs.Stat;
import org.json.JSONObject;
import org.junit.Test;

import javax.security.auth.Subject;
import java.util.Map;

import static org.junit.Assert.assertEquals;

public class InodeMetadataTest {
    @Test
    public void testAsJsonObject() throws Exception {
        InodeMetadata metadata = new InodeMetadata(Stat.Type.DIRECTORY, new Subject(), 0644, 42, 43)
                .withLink(100, "foo")
                .withLink(200, "bar");
        JSONObject jsonObject = metadata.asJsonObject();
        InodeMetadata thawed = new InodeMetadata(jsonObject);
        assertEquals(metadata, thawed);
        Map<Long, String> links = metadata.getLinks();
        assertEquals(2, links.size());
        assertEquals("foo", links.get(100l));
        assertEquals("bar", links.get(200l));
    }
}