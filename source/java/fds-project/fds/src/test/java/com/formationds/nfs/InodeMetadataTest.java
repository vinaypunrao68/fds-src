package com.formationds.nfs;

import com.google.common.base.Joiner;
import com.google.common.collect.Multimap;
import org.dcache.nfs.v4.xdr.*;
import org.dcache.nfs.vfs.Stat;
import org.json.JSONObject;
import org.junit.Test;

import javax.security.auth.Subject;

import static org.junit.Assert.*;

public class InodeMetadataTest {
    public static final String JSON_FRAME = "{\n" +
            "    \"NFS_FILE_ID\": 42,\n" +
            "    \"NFS_ATIME\": 1447180306863,\n" +
            "    \"NFS_MTIME\": 1447180306863,\n" +
            "    \"NFS_MODE\": 420,\n" +
            "    \"NFS_SIZE\": 0,\n" +
            "    \"NFS_GID\": 0,\n" +
            "    \"NFS_CTIME\": 1447180306863,\n" +
            "    \"NFS_TYPE\": \"DIRECTORY\",\n" +
            "    \"NFS_LINKS\": [\n" +
            "        {\n" +
            "            \"parent\": 100,\n" +
            "            \"name\": \"foo\"\n" +
            "        },\n" +
            "        {\n" +
            "            \"parent\": 100,\n" +
            "            \"name\": \"bar\"\n" +
            "        },\n" +
            "        {\n" +
            "            \"parent\": 200,\n" +
            "            \"name\": \"panda\"\n" +
            "        }\n" +
            "    ],\n" +
            "    \"NFS_UID\": -1,\n" +
            "    \"NFS_GENERATION\": 0,\n" +
            "    \"NFS_ACES\": []\n" +
            "}";

    @Test
    public void testAcesAreOptional() throws Exception {
        InodeMetadata metadata = new InodeMetadata(new JSONObject(JSON_FRAME));
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
    public void testLinks() throws Exception {
        InodeMetadata metadata = new InodeMetadata(Stat.Type.DIRECTORY, new Subject(), 0644, 42)
                .withLink(100, "foo")
                .withLink(100, "bar")
                .withLink(200, "panda");
        assertEquals(3, metadata.refCount());
        assertTrue(metadata.hasLink(100, "foo"));
        assertTrue(metadata.hasLink(100, "bar"));
        assertTrue(metadata.hasLink(200, "panda"));
        assertFalse(metadata.hasLink(100, "panda"));

        metadata = metadata.withoutLink(100, "foo");
        assertFalse(metadata.hasLink(100, "foo"));
        assertTrue(metadata.hasLink(100, "bar"));
        assertEquals(2, metadata.refCount());
    }

    @Test
    public void testAsJsonObject() throws Exception {
        InodeMetadata metadata = new InodeMetadata(Stat.Type.DIRECTORY, new Subject(), 0644, 42)
                .withLink(100, "foo")
                .withLink(200, "bar")
                .withLink(200, "hello");
        JSONObject jsonObject = metadata.asJsonObject();
        InodeMetadata thawed = new InodeMetadata(jsonObject);
        assertEquals(metadata, thawed);
        Multimap<Long, String> links = metadata.getLinks();
        assertEquals("foo", Joiner.on(", ").join(links.get(100l)));
        assertEquals("bar, hello", Joiner.on(", ").join(links.get(200l)));
    }
}