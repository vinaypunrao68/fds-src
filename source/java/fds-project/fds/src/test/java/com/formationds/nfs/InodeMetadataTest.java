package com.formationds.nfs;

import org.dcache.nfs.v4.xdr.*;
import org.dcache.nfs.vfs.Stat;
import org.json.JSONObject;
import org.junit.Test;

import javax.security.auth.Subject;
import java.util.Map;

import static org.junit.Assert.assertEquals;

public class InodeMetadataTest {
    @Test
    public void testAcesAreOptional() throws Exception {
        String oldFrame = "{\n" +
                "    \"NFS_FILE_ID\": 42,\n" +
                "    \"NFS_ATIME\": 1440537846161,\n" +
                "    \"NFS_MTIME\": 1440537846161,\n" +
                "    \"NFS_MODE\": 420,\n" +
                "    \"NFS_SIZE\": 0,\n" +
                "    \"NFS_GID\": 0,\n" +
                "    \"NFS_CTIME\": 1440537846161,\n" +
                "    \"NFS_TYPE\": \"DIRECTORY\",\n" +
                "    \"NFS_LINKS\": {\n" +
                "        \"100\": \"foo\",\n" +
                "        \"200\": \"bar\"\n" +
                "    },\n" +
                "    \"NFS_UID\": -1,\n" +
                "    \"NFS_GENERATION\": 0,\n" +
                "    \"NFS_VOLUME_ID\": 43\n" +
                "}";
        InodeMetadata metadata = new InodeMetadata(new JSONObject(oldFrame));
        assertEquals(0, metadata.getNfsAces().length);
        nfsace4 nfsace4 = new nfsace4();
        nfsace4.type = new acetype4(new uint32_t(1));
        nfsace4.access_mask = new acemask4(new uint32_t(2));
        nfsace4.flag = new aceflag4(new uint32_t(3));
        nfsace4.who = new utf8str_mixed("panda");
        metadata = metadata.withNfsAces(new nfsace4[]{nfsace4});
        JSONObject newFormat = metadata.asJsonObject();
        assertEquals(metadata, new InodeMetadata(newFormat));
    }

    @Test
    public void testAsJsonObject() throws Exception {
        InodeMetadata metadata = new InodeMetadata(Stat.Type.DIRECTORY, new Subject(), 0644, 42, 43)
                .withLink(100, "foo")
                .withLink(200, "bar");
        JSONObject jsonObject = metadata.asJsonObject();
        System.out.println(jsonObject.toString(4));
        InodeMetadata thawed = new InodeMetadata(jsonObject);
        assertEquals(metadata, thawed);
        Map<Long, String> links = metadata.getLinks();
        assertEquals(2, links.size());
        assertEquals("foo", links.get(100l));
        assertEquals("bar", links.get(200l));
    }
}