package com.formationds.nfs;

import com.formationds.util.FunctionWithExceptions;
import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import org.apache.log4j.Logger;
import org.apache.lucene.analysis.standard.StandardAnalyzer;
import org.apache.lucene.document.Document;
import org.apache.lucene.index.DirectoryReader;
import org.apache.lucene.index.IndexWriter;
import org.apache.lucene.index.IndexWriterConfig;
import org.apache.lucene.search.*;
import org.apache.lucene.store.Directory;
import org.apache.lucene.store.RAMDirectory;
import org.dcache.nfs.vfs.DirectoryEntry;
import org.dcache.nfs.vfs.Inode;

import java.io.IOException;
import java.util.*;
import java.util.concurrent.TimeUnit;

public class InodeIndex {
    private static final Logger LOG = Logger.getLogger(InodeIndex.class);

    private final FunctionWithExceptions<Long, Directory> directoryProvider;
    private final ExportResolver exportResolver;
    private final Cache<Long, Channels> cachedChannels;

    private static class Channels {
        private IndexWriter writer = null;
        private DirectoryReader reader = null;
        private Directory directory;

        public Channels(Directory directory) {
            this.directory = directory;
        }

        public synchronized IndexWriter writer() throws IOException {
            if (writer == null) {
                writer = new IndexWriter(directory, new IndexWriterConfig(new StandardAnalyzer()));
            }
            return writer;
        }

        public synchronized IndexSearcher searcher() throws IOException {
            if (reader == null) {
                reader = DirectoryReader.open(directory);
            }

            return new IndexSearcher(reader);
        }

        public synchronized void refresh() {
            reader = null;
        }
    }

    public InodeIndex(ExportResolver exportResolver) {
        this(new FunctionWithExceptions<Long, Directory>() {
            Map<Long, Directory> directories = new HashMap<>();

            @Override
            public Directory apply(Long id) {
                return directories.compute(id, (k, v) -> {
                    if (v == null) {
                        v = new RAMDirectory();
                    }
                    return v;
                });
            }
        }, exportResolver);
    }

    public InodeIndex(FunctionWithExceptions<Long, Directory> provider, ExportResolver exportResolver) {
        this.directoryProvider = provider;
        this.exportResolver = exportResolver;
        this.cachedChannels = CacheBuilder.newBuilder().expireAfterAccess(10, TimeUnit.MINUTES).build();
    }

    public Optional<InodeMetadata> lookup(Inode parent, String name) throws IOException {
        Channels channels = null;
        if (InodeMap.isRoot(parent)) {
            if (!exportResolver.exists(name)) {
                return Optional.empty();
            }
            channels = openChannels(exportResolver.exportId(name));
        } else {
            channels = openChannels(parent);
        }

        IndexSearcher indexSearcher = channels.searcher();
        Query query = InodeMetadata.lookupQuery(InodeMetadata.fileId(parent), name);
        TopDocs results = null;
        try {
            results = indexSearcher.search(query, indexSearcher.getIndexReader().maxDoc());
        } catch (IOException e) {
            LOG.error("Error running query " + query + " on volume " + name, e);
            throw e;
        }
        if (results.totalHits == 0) {
            return Optional.empty();
        } else {
            Document document = indexSearcher.doc(results.scoreDocs[0].doc);
            return Optional.of(new InodeMetadata(document));
        }
    }

    public List<DirectoryEntry> list(Inode inode) throws IOException {
        IndexSearcher indexSearcher = openChannels(inode).searcher();
        long parentId = InodeMetadata.fileId(inode);
        Query query = InodeMetadata.listQuery(parentId);
        TopFieldDocs tfd = null;
        try {
            tfd = indexSearcher.search(query, indexSearcher.getIndexReader().maxDoc(), Sort.INDEXORDER);
        } catch (IOException e) {
            LOG.error("Error running query " + query + " on volume " + exportResolver.volumeName(inode.exportIndex()), e);
            throw e;
        }
        List<DirectoryEntry> results = new ArrayList<>(tfd.totalHits);
        for (int i = 0; i < tfd.totalHits; i++) {
            int docId = tfd.scoreDocs[i].doc;
            InodeMetadata inodeMetadata = new InodeMetadata(indexSearcher.doc(docId));
            results.add(inodeMetadata.asDirectoryEntry(parentId));
        }

        return results;
    }

    public void index(long volumeId, InodeMetadata... entries) throws IOException {
        Channels channels = this.openChannels(volumeId);
        IndexWriter indexWriter = channels.writer();
        for (InodeMetadata entry : entries) {
            indexWriter.updateDocument(entry.identity(), entry.asDocument());
        }
        indexWriter.commit();
        channels.refresh();
    }

    public void remove(Inode inode) throws IOException {
        Channels channels = openChannels(inode);
        IndexWriter indexWriter = channels.writer();
        indexWriter.deleteDocuments(InodeMetadata.identity(inode));
        indexWriter.commit();
        channels.refresh();
    }


    private Channels openChannels(Inode parent) throws IOException {
        return openChannels((long) parent.exportIndex());
    }

    private Channels openChannels(long exportId) throws IOException {
        try {
            return cachedChannels.get(exportId, () -> {
                Channels channels = new Channels(directoryProvider.apply(exportId));
                LOG.debug("Channels: " + channels + ", exportId=" + exportId);
                return channels;
            });
        } catch (Exception e) {
            LOG.error("Error opening directory", e);
            throw new IOException(e);
        }
    }
}
