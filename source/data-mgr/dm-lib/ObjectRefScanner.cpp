/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <sstream>
#include <lib/Catalog.h>
#include <ObjectRefScanner.h>
#include <DataMgr.h>
#include <util/stringutils.h>

namespace fds {

inline std::string volTokBloomFilterKey(const fds_volid_t& volId, 
                                     const fds_token_id &tokenId)
{
    return util::strformat("vol%ld_tok%d", volId, tokenId);
}

inline std::string aggrBloomFilterKey(const fds_token_id &tokenId)
{
    return util::strformat("aggr_tok%d", tokenId);
}

BloomFilterStore::BloomFilterStore(const std::string &path, uint32_t cacheSize)
: basePath(path),
  maxCacheSize(cacheSize),
  bloomfilterBits(1*MB),
  accessCnt(1)
{
}

BloomFilterStore::~BloomFilterStore() {
}

util::BloomFilterPtr BloomFilterStore::load(const std::string &key) {
    auto deserializer = serialize::getFileDeserializer(basePath + key);
    auto bloomfilter = util::BloomFilterPtr(new util::BloomFilter(bloomfilterBits));
    auto readSize = bloomfilter->read(deserializer);
    fds_verify(readSize == bloomfilterBits/8);
    return bloomfilter;
}

void BloomFilterStore::save(const std::string &key, util::BloomFilterPtr bloomfilter) {
    auto serializer = serialize::getFileSerializer(basePath + key);
    auto writeSize = bloomfilter->write(serializer);
    fds_verify(writeSize == bloomfilterBits/8);
}

void BloomFilterStore::addToCache(const std::string &key, util::BloomFilterPtr bloomfilter) {
    BFNode newNode;
    newNode.accessCnt = accessCnt;
    newNode.key = key;
    newNode.bloomfilter = bloomfilter;
    cache.push_front(newNode);
    accessCnt++;

    /* Evict an entry if needed */
    if (cache.size() >= maxCacheSize) {
        auto &back = cache.back();
        save(back.key, back.bloomfilter);
        cache.pop_back();
    }
    fds_assert(cache.size() < maxCacheSize);
}

util::BloomFilterPtr BloomFilterStore::getFromCache(const std::string &key) {
    auto itr = cache.begin();
    for (; itr != cache.end(); itr++) {
        if (itr->key == key) {
            break;
        }
    }
    if (itr == cache.end()) {
        return nullptr;
    }

    /* Update the access count and move the found entry to front of the cache */
    itr->accessCnt = accessCnt;
    accessCnt++;
    cache.splice(cache.begin(), cache, itr);
    return cache.begin()->bloomfilter;
}

util::BloomFilterPtr BloomFilterStore::get(const std::string &key, bool create) {
    util::BloomFilterPtr bloomfilter;
    /* Check in the index */
    auto itr = index.find(key);
    if (itr == index.end()) {
        if (!create) {
            return bloomfilter;
        }
        /* Create empty bloomfilter and add to index and cache */
        bloomfilter.reset(new util::BloomFilter(bloomfilterBits));
        addToCache(key, bloomfilter);
        index.insert(key);
    } else {
        /* Check in cache */
        auto bloomfilter = getFromCache(key);
        if (!bloomfilter) {
            bloomfilter = load(key);
            addToCache(key, bloomfilter);
        }
    }
    return bloomfilter;
}

ObjectRefMgr::ObjectRefMgr(CommonModuleProviderIf *moduleProvider, DataMgr* dm)
: HasModuleProvider(moduleProvider),
  Module("RefcountScanner"),
  dataMgr(dm),
  qosHelper(*dm)
{
    // TODO(Rao): assign other memebers
}

void ObjectRefMgr::mod_startup() {
    auto dmUserRepo = MODULEPROVIDER()->proc_fdsroot()->dir_user_repo_dm();
    bfStore.reset(new BloomFilterStore(util::strformat("%s/bloomfilters/", dmUserRepo.c_str()), 5));

    auto timer = MODULEPROVIDER()->getTimer();
    scanTask = boost::shared_ptr<FdsTimerTask>(
        new FdsTimerFunctionTask(
            *timer,
            [this] () {
            /* Immediately post to threadpool so we don't hold up timer thread */
            auto req = new DmFunctor(FdsDmSysTaskId, std::bind(&ObjectRefMgr::scan, this));
            qosHelper.addToQueue(req);
            }));
    timer->schedule(scanTask, scanIntervalSec);
    // TODO(Rao): Set members based on config
    // TODO(Rao): schdule scanner based on config
}

void ObjectRefMgr::mod_shutdown() {
}

// TODO(Rao): Check if we need any synchronization
void ObjectRefMgr::scan() {
    if (scanList.size() == 0) {
        buildScanList();
    }

    if (currentItr != scanList.end()) {
        /* Scan step */
        Error e = currentItr->scan();
        if (e != ERR_OK) {
            /* Report the error and move on */
            GLOGWARN<< "Failed to scan: " << currentItr->logString() << " " << e;
            currentItr->finishScan(e);
            currentItr++;
        } else {
            if (currentItr->isComplete()) {
                currentItr->finishScan(ERR_OK);
                currentItr++;
            }
        }
    }

    if (currentItr != scanList.end()) {
        /* Schedule next scan step */
        auto req = new DmFunctor(FdsDmSysTaskId, std::bind(&ObjectRefMgr::scan, this));
        qosHelper.addToQueue(req);
    } else {
        /* Schedule another scan cycle */
        auto timer = MODULEPROVIDER()->getTimer();
        timer->schedule(scanTask, scanIntervalSec);
    }
}

void ObjectRefMgr::buildScanList() {
    currentDlt = MODULEPROVIDER()->getSvcMgr()->getCurrentDLT();
    // TODO(Rao): Get list of volumes and create volume contexts for each volume
}

VolumeRefScannerContext::VolumeRefScannerContext(ObjectRefMgr* m, fds_volid_t vId)
: ObjectRefScanner(m)
{
    volId = vId;
    std::stringstream ss;
    ss << "VolumeRefScannerContext:" << vId;
    logStr = ss.str();

    scanners.push_back(ObjectRefScannerPtr(new VolumeObjectRefScanner(m, vId)));
    // TODO(Rao): Add snapshots as well
    itr = scanners.begin();
    state = SCANNING;
}

Error VolumeRefScannerContext::scan() {
    Error e = ERR_OK;
    if (itr != scanners.end()) {
        auto scanner = *itr;
        e = scanner->scan();
        if (e != ERR_OK) {
            GLOGWARN<< "Failed to scan: " << scanner->logString() << " " << e;
            scanner->finishScan(e);
            itr++;
        } else {
            if (scanner->isComplete()) {
                scanner->finishScan(ERR_OK);
                itr++;
            }
        }
    }
    return e;
}

Error VolumeRefScannerContext::finishScan(const Error &e) {
    state = COMPLETE;
    completionError = e;
    if (e != ERR_OK) {
        /* Scan completed with an error.  Nothing more to do */
        return ERR_OK;
    }

    /* For each token update the aggregate bloom filter */
    auto bfStore = objRefMgr->getBloomFiltersStore();
    auto tokenCnt = objRefMgr->getCurrentDlt()->getNumTokens();
    for (uint32_t token = 0; token < tokenCnt; token++) {
        auto bloomfilter = bfStore->get(volTokBloomFilterKey(volId, token), false);
        if (!bloomfilter) continue;
        auto aggrbloomfilter = bfStore->get(aggrBloomFilterKey(token));
        aggrbloomfilter->merge(*bloomfilter);
    }
    objRefMgr->getScanSuccessVols().push_back(volId);
    return ERR_OK;
}

bool VolumeRefScannerContext::isComplete() const {
    return (state == COMPLETE || itr == scanners.end());
}

std::string VolumeRefScannerContext::logString() const {
    return logStr;
}

VolumeObjectRefScanner::VolumeObjectRefScanner(ObjectRefMgr* m, fds_volid_t vId)
: ObjectRefScanner(m),
    bInited(false),
    volId(vId)
{
    std::stringstream ss;
    ss << "VolumeObjectRefScanner:" << volId;
    logStr = ss.str();
}

Error VolumeObjectRefScanner::init() {
    /* Create snapshot */
    auto volcatIf = objRefMgr->getDataMgr()->timeVolCat_->queryIface();
    auto err = volcatIf->getVolumeSnapshot(volId, snap);
    if (err != ERR_OK) {
        return err;
    }
    // TODO(Rao): Start the iterator based on snapshot
    // We should lock the volume as well to protect against deletions
    return err;
}

Error VolumeObjectRefScanner::scan() {
    if (state == INIT) {
        auto error = init();
        if (error != ERR_OK) {
            GLOGWARN << "Failed to initialize " << logString();
            return error;
        }
        state = SCANNING;
    }

    auto currentDlt = objRefMgr->getCurrentDlt();
    std::map<fds_token_id, std::list<ObjectID>> tokenObjects;
    std::list<ObjectID> objects;
    auto volcatIf = objRefMgr->getDataMgr()->timeVolCat_->queryIface();
    volcatIf->getObjectIds(volId, objRefMgr->getMaxEntriesToScan(), snap, itr, objects);

    /* Bucketize the objects into tokens */
    for (const auto &oid : objects) {
        tokenObjects[currentDlt->getToken(oid)].push_back(oid);
    }
    objects.clear();

    /* Update token bloomfilter one at a time.  We do this because bloomfilters can be
     * quite large.  Processing one bloomfilter at a timer allows us to efficient with
     * memory usage
     */
    auto bfStore = objRefMgr->getBloomFiltersStore();
    for (const auto &kv : tokenObjects) {
        auto bloomfilter = bfStore->get(volTokBloomFilterKey(volId, kv.first));
        auto &objects = kv.second;
        for (const auto &oid : objects) {
            bloomfilter->add(oid);
        }
    }
    
    return ERR_OK;
}

Error VolumeObjectRefScanner::finishScan(const Error &e) {
    state = COMPLETE;
    completionError = e;
    auto volcatIf = objRefMgr->getDataMgr()->timeVolCat_->queryIface();
    auto err = volcatIf->freeVolumeSnapshot(volId, snap);
    if (err != ERR_OK) {
       GLOGWARN << "Failed to release snapshot for volId: " << volId; 
    }

    return err;
}

}  // namespace fds
