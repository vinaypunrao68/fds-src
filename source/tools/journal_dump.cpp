// Copyright (c) 2012 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

// Standard includes.
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// System includes.
#include <boost/scoped_ptr.hpp>

// Internal includes.
#include "catalogKeys/BlobMetadataKey.h"
#include "catalogKeys/BlobObjectKey.h"
#include "catalogKeys/CatalogKeyType.h"
#include "db/dbformat.h"
#include "db/filename.h"
#include "db/log_reader.h"
#include "db/version_edit.h"
#include "db/write_batch_internal.h"
#include "dm-vol-cat/DmPersistVolCat.h"
#include "leveldb/cat_journal.h"
#include "leveldb/env.h"
#include "leveldb/iterator.h"
#include "leveldb/options.h"
#include "leveldb/status.h"
#include "leveldb/table.h"
#include "leveldb/write_batch.h"
#include "port/port.h"
#include "util/logging.h"
#include "DmBlobTypes.h"

#define USE_NEW_LDB_STRUCTURES

namespace leveldb {

namespace {

// Print contents of a log file. (*func)() is called on every record.
bool PrintLogContents(Env* env, const std::string& fname, void (*func)(Slice)) {
    SequentialFile* file;
    Status s = env->NewSequentialFile(fname, &file);
    if (!s.ok()) {
        fprintf(stderr, "%s\n", s.ToString().c_str());
        return false;
    }
    CorruptionReporter reporter;
    log::Reader reader(file, &reporter, true, 0);
    Slice record;
    std::string scratch;
    while (reader.ReadRecord(&record, &scratch)) {
        printf("--- offset %llu; ",
               static_cast<unsigned long long>(reader.LastRecordOffset())); //NOLINT
        (*func)(record);
    }
    delete file;
    return true;
}

// Called on every item found in a WriteBatch.
class WriteBatchItemPrinter : public WriteBatch::Handler {
  public:
    uint64_t offset_;
    uint64_t sequence_;

    virtual void Put(const Slice& key, const Slice& value) {
#ifdef USE_NEW_LDB_STRUCTURES
        fds::CatalogKeyType keyType = *reinterpret_cast<fds::CatalogKeyType const*>(key.data());
        switch (keyType) {
            case fds::CatalogKeyType::JOURNAL_TIMESTAMP:
                std::cout << "=> Timestamp: " << *reinterpret_cast<fds_uint64_t const*>(value.data())
                          << "\n";
                break;
            case fds::CatalogKeyType::BLOB_OBJECTS: {
                BlobObjectKey blobObjectKey { key };
                std::cout << "=> put [blob=" << blobObjectKey.getBlobName()
                          << " index=" << blobObjectKey.getObjectIndex()
                          << " obj=" << fds::ObjectID(reinterpret_cast<uint8_t const*>(value.data()),
                                                      value.size()).ToHex()
                          << "]\n";
                break;
            }
            case fds::CatalogKeyType::BLOB_METADATA: {
                fds::BlobMetaDesc blobMeta;
                blobMeta.loadSerialized(std::string{value.data(), value.size()});

                std::cout << "=> put meta [blob=" << blobMeta.desc.blob_name
                          << " size=" << blobMeta.desc.blob_size
                          << " version=" << blobMeta.desc.version
                          << " seq=" << blobMeta.desc.sequence_id
                          << "]\n";

                std::cout << "  [ ";
                for (auto const& it : blobMeta.meta_list) {
                    std::cout << it.first << "=" << it.second << " ";
                }
                std::cout << "]\n";

                break;
            }
            case fds::CatalogKeyType::VOLUME_METADATA: {
                const fpi::FDSP_MetaDataList metadataList;
                const sequence_id_t seq_id=0;
                VolumeMetaDesc volDesc(metadataList, seq_id);
                volDesc.loadSerialized(std::string{value.data(), value.size()});
                std::cout << "=> put volume meta: " ;
                std::cout << "[seqid=" << volDesc.sequence_id << " " ;
                for (const auto& item : volDesc.meta_list) {
                    std::cout << item.first << "=" << item.second << " ";
                }
                std::cout << "]\n";
                break;
            }
            default:
                throw std::runtime_error{"Unrecognized key type: " + std::to_string(static_cast<unsigned int>(keyType)) + "."};
        }
#else
        std::string keyStr(key.data(), key.size());
        std::string dataStr(value.data(), value.size());

        fds::ExtentKey extKey;
        extKey.loadSerialized(keyStr);

        boost::scoped_ptr<fds::BlobExtent> extent;
        if (extKey.extent_id) {
            extent.reset(new fds::BlobExtent(extKey.extent_id, 2048, 0, 2048));
        } else {
            extent.reset(new fds::BlobExtent0(extKey.blob_name, 2048, 0, 2048));
        }

        extent->loadSerialized(dataStr);

        std::cout << "\n=> put 'Blob=" << extKey.blob_name << ", Extent=" << extKey.extent_id
                  << "'" << std::endl;
        std::vector<fds::ObjectID> oids;
        extent->getAllObjects(oids);
        for (unsigned i = 0; i < oids.size(); ++i) {
            std::cout << "[Index: " << i << " -> " << oids[i] << "]\n";
        }
#endif
    }

    virtual void Delete(const Slice& key) {
#ifdef USE_NEW_LDB_STRUCTURES
        fds::CatalogKeyType keyType = *reinterpret_cast<fds::CatalogKeyType const*>(key.data());
        switch (keyType) {
            case fds::CatalogKeyType::JOURNAL_TIMESTAMP:
                std::cout << "=> del JournalTimestampKey\n";
                break;
            case fds::CatalogKeyType::BLOB_OBJECTS: {
                BlobObjectKey blobObjectKey { key };
                std::cout << "=> del [blob=" << blobObjectKey.getBlobName()
                          << " index=" << blobObjectKey.getObjectIndex() << "]\n";
                break;
            }
            case fds::CatalogKeyType::BLOB_METADATA: {
                BlobMetadataKey blobMetaKey { key };
                std::cout << "=> del [blobmeta=" << blobMetaKey.getBlobName() << "]\n";
                break;
            }
            case fds::CatalogKeyType::VOLUME_METADATA: {
                std::cout << "=> del [volumeMeta]\n";
                break;
            }

            default:
                throw std::runtime_error{"Unrecognized key type: "
                            + std::to_string(static_cast<unsigned int>(keyType)) + "."};
        }
#else
        std::string keyStr(key.data(), key.size());
        fds::ExtentKey extKey;
        extKey.loadSerialized(keyStr);
        std::cout << "  del 'Blob=" << extKey.blob_name << ", Extent=" << extKey.extent_id
                  << "'" << std::endl;
#endif
    }
};


// Called on every log record (each one of which is a WriteBatch)
// found in a kLogFile.
static void WriteBatchPrinter(Slice record) {
    if (record.size() < 12) {
        printf("log record length %d is too small\n",
               static_cast<int>(record.size()));
        return;
    }
    WriteBatch batch;
    WriteBatchInternal::SetContents(&batch, record);
    printf("sequence %llu\n",
           static_cast<unsigned long long>(WriteBatchInternal::Sequence(&batch))); //NOLINT
    WriteBatchItemPrinter batch_item_printer;
    Status s = batch.Iterate(&batch_item_printer);
    std::cout << std::endl;
    if (!s.ok()) {
        printf("  error: %s\n", s.ToString().c_str());
    }
}

bool DumpLog(Env* env, const std::string& fname) {
    return PrintLogContents(env, fname, WriteBatchPrinter);
}

bool HandleDumpCommand(Env* env, char** files, int num) {
    bool ok = true;
    for (int i = 0; i < num; i++) {
        ok &= DumpLog(env, files[i]);
    }
    return ok;
}

} //NOLINT
}  // namespace leveldb

static void Usage() {
    std::cout << "Usage: journal-dump <files>...\n"
              << "   files...         -- dump contents of specified journal files\n";
}

int main(int argc, char** argv) {
    leveldb::Env* env = leveldb::Env::Default();
    bool ok = true;
    if (argc < 2) {
        Usage();
        ok = false;
    } else {
        std::string command("dump");
        if (command == "dump") {
            // ok = leveldb::HandleDumpCommand(env, argv+1, argc-1);
            leveldb::CatJournalIterator iter(argv[1]);
            leveldb::WriteBatchItemPrinter batch_item_printer;
            for (; iter.isValid(); iter.Next()) {
                const leveldb::WriteBatch &wb = iter.GetBatch();
                leveldb::Status s = wb.Iterate(&batch_item_printer);
                std::cout << std::endl;
                if (!s.ok()) {
                    printf("  error: %s\n", s.ToString().c_str());
                }
            }
        } else {
            Usage();
            ok = false;
        }
    }
    return (ok ? 0 : 1);
}
