package com.formationds.sc;

import com.formationds.util.Lazy;

import java.util.HashMap;
import java.util.Map;

public enum FdsError {
    ERR_OK(0, "ALL OK"),
    ERR_DUPLICATE(1, "Data is a duplicate"),
    ERR_HASH_COLLISION(2, "Hash digest collision"),
    ERR_DISK_WRITE_FAILED(3, "Unable to write data to disk"),
    ERR_DISK_READ_FAILED(4, "Unable to read data from disk"),
    ERR_CAT_QUERY_FAILED(5, "Unable to query into catalog"),
    ERR_CAT_ENTRY_NOT_FOUND(6, "Catalog entry not found"),
    ERR_INVALID_ARG(7, "Invalid argument or parameter"),
    ERR_PENDING_RESP(8, "Response is pending"),
    ERR_NOT_FOUND(9, "Not found"),
    ERR_VOL_ADMISSION_FAILED(10, "Admission control for a volume failed"),
    ERR_GET_DLT_FAILED(11, "Get DLT failed"),
    ERR_GET_DMT_FAILED(12, "Get DMT failed"),
    ERR_NOT_IMPLEMENTED(13, "Feature not implemented"),
    ERR_OUT_OF_MEMORY(14, "Out of memory"),
    ERR_DUPLICATE_UUID(15, "Node name hashed to duplicate UUID"),
    ERR_TRANS_JOURNAL_OUT_OF_IDS(16, "Ran out of transaction ids"),
    ERR_TRANS_JOURNAL_REQUEST_QUEUED(17, "Transaction request is queued"),
    ERR_NODE_NOT_ACTIVE(18, "Node is not in active state"),
    ERR_VOL_NOT_EMPTY(19, "Volume is not empty"),
    ERR_BLOB_NOT_FOUND(20, "Blob does not exist"),
    ERR_NOT_READY(21, "Not ready, re-try later"),
    ERR_INVALID_DLT(22, "Invalid DLT"),
    ERR_PERSIST_STATE_MISMATCH(23, "Mismatch in persistent state"),
    ERR_EXCEED_MIN_IOPS(24, "Operation not permitted: would exceed total min iops"),
    ERR_UNAUTH_ACCESS(25, "Unauthorized access"),
    ERR_DISK_FILE_UNLINK_FAILED(26, "Failed to unlink file"),
    ERR_SERIALIZE_FAILED(27, "Failed to serialize data"),
    ERR_NO_BYTES_WRITTEN(28, "No bytes were written"),
    ERR_NO_BYTES_READ(29, "No bytes were read"),
    ERR_VOL_NOT_FOUND(30, "Volume not found"),
    ERR_VOL_DUPLICATE(31, "Duplicate volume"),
    ERR_NETWORK_CORRUPT(32, "Object Buffer corrupted network"),
    ERR_ONDISK_DATA_CORRUPT(33, "OnDisk Object Buffer corrupt"),
    ERR_INVALID_DMT(34, "Invalid DMT"),
    ERR_CATSYNC_NOT_PROGRESS(35, "Catalog sync is not in progress"),
    ERR_DM_RSYNC_FAILED(36, "Rsync failed to sync volume catalog in DM"),
    ERR_DMT_FORWARD(37, "Forward DM request"),
    ERR_DM_TX_EXISTS(38, "Transacton already exists in DM commit log"),
    ERR_DM_TX_NOT_STARTED(39, "Transaction not started"),
    ERR_DM_TX_COMMITTED(40, "Transaction already committed"),
    ERR_DM_OFFSET_OUT_RANGE(41, "Offset in blob out of range"),
    ERR_DM_TX_ACTIVE(42, "Transaction active"),
    ERR_DM_MAX_CL_ENTRIES(43, "Reached max commit log entries"),
    ERR_DM_VOL_MARKED_DELETED(44, "Volume marked as deleted"),
    ERR_DM_OP_NOT_ALLOWED(45, "Operation not allowed"),
    ERR_DM_SNAPSHOT_FAILED(46, "Snapshot failed"),
    ERR_INVALID_VOL_ID(47, "Invalid volume ID"),
    ERR_DM_JOURNAL_TIME(48, "Journal time not available"),
    ERR_DM_REPLAY_JOURNAL(49, "Unable to replay journal for provided interval"),
    ERR_DM_FORWARD_FAILED(50, "DM forwarding failed"),
    ERR_SHUTTING_DOWN(51, "Service is shutting down"),
    ERR_DM_MIGRATION_ABORTED(52, "DM migration aborted"),
    ERR_DM_VOL_NOT_ACTIVATED(53, "DM volume not activated"),
    ERR_DM_INVALID_TX_ID(54, "Invalid transaction ID"),
    ERR_IO_PENDING(55, "There are inflight IOs for the current Table version"),
    ERR_ARCHIVE_SNAP_TAR_FAILED(56, "Failed to tar snap"),
    ERR_ARCHIVE_PUT_FAILED(57, "Archive put failed"),
    ERR_ARCHIVE_GET_FAILED(58, "Archive get failed"),
    ERR_VOLUME_ACCESS_DENIED(59, "Volume access denied"),
    ERR_TOKEN_NOT_READY(60, "Token not ready"),
    ERR_DM_INVALID_REGEX(61, "DM invalid regex"),
    ERR_DM_NOT_PRIMARY(62, "Not one of the primaries"),
    ERR_DM_NOT_TOP_PRIMARY(63, "Not the first primary"),
    ERR_SERVICE_CAPACITY_FULL(64, "Service ran out of disk capacity"),
    ERR_SERVICE_CAPACITY_DANGEROUS(65, "Service is at dangerous used disk capacity level"),
    ERR_DM_UNRECOGNIZED_PATTERN_SEMANTICS(66, "Pattern semantics not recognized"),
    ERR_TIMER_TASK_NOT_SCHEDULED(67, "Timer task was not scheduled"),

    ERR_IO_DLT_MISMATCH(100, ""),
    ERR_IO_DMT_MISMATCH(101, ""),

    ERR_BLOB_OFFSET_INVALID(500, ""),
    ERR_BLOB_SEQUENCE_ID_REGRESSION(501, ""),

    ERR_MIGRATION_DUPLICATE_REQUEST(1000, ""),
    ERR_DM_CAT_MIGRATION_ABORTED(1001, ""),
    ERR_DM_CAT_MIGRATION_TIMEOUT(1002, ""),
    ERR_DM_CAT_MIGRATION_DIFF_FAILED(1003, ""),
    ERR_DM_CAT_MIGRATION_APPLY_BLOB_OFFSET_FAILURE(1004, ""),
    ERR_DM_CAT_MIGRATION_APPLY_BLOB_DESC_FAILURE(1005, ""),
    ERR_DM_CAT_MIGRATION_DEST_MSG_CORRUPT(1006, ""),
    ERR_DM_CAT_MIGRATION_SOURCE_MSG_CORRUPT(1007, ""),

    ERR_FAR_INVALID_REQUEST(1500, ""),
    ERR_FAR_SHUTDOWN(1501, ""),

    ERR_SM_NOT_IN_SYNC_MODE(2000, ""),
    ERR_SM_TOKENSTATEDB_KEY_NOT_FOUND(2001, ""),
    ERR_SM_TOKENSTATEDB_DUPLICATE_KEY(2002, ""),
    ERR_SM_OBJECT_DATA_MISSING(2003, ""),
    ERR_SM_ZERO_REFCNT_OBJECT(2004, ""),
    ERR_SM_OLT_DISKS_INCONSISTENT(2005, ""),
    ERR_SM_SUPERBLOCK_MISSING_FILE(2006, ""),
    ERR_SM_SUPERBLOCK_CHECKSUM_FAIL(2007, ""),
    ERR_SM_SUPERBLOCK_DATA_CORRUPT(2008, ""),
    ERR_SM_SUPERBLOCK_READ_FAIL(2009, ""),
    ERR_SM_SUPERBLOCK_WRITE_FAIL(2010, ""),
    ERR_SM_SUPERBLOCK_NO_RECONCILE(2011, ""),
    ERR_SM_SUPERBLOCK_INCONSISTENT(2012, ""),
    ERR_SM_NOERR_PRISTINE_STATE(2013, ""),
    ERR_SM_GC_ENABLED(2014, ""),
    ERR_SM_GC_TEMP_DISABLED(2015, ""),
    ERR_SM_AUTO_GC_FAILED(2016, ""),
    ERR_SM_DUP_OBJECT_CORRUPT(2017, ""),
    ERR_SM_SHUTTING_DOWN(2018, ""),
    ERR_SM_TOK_MIGRATION_DESTINATION_MSG_CORRUPT(2019, ""),
    ERR_SM_TOK_MIGRATION_SOURCE_MSG_CORRUPT(2020, ""),
    ERR_SM_TOK_MIGRATION_NO_DATA_RECVD(2021, ""),
    ERR_SM_TOK_MIGRATION_DATA_MISMATCH(2022, ""),
    ERR_SM_TOK_MIGRATION_METADATA_MISMATCH(2023, ""),
    ERR_SM_TOK_MIGRATION_ABORTED(2024, ""),
    ERR_SM_TOK_MIGRATION_TIMEOUT(2025, ""),
    ERR_SM_TOK_MIGRATION_INPROGRESS(2026, ""),
    ERR_SM_TOK_MIGRATION_SRC_SVC_REQUEST(2027, ""),
    ERR_SM_TIER_WRITEBACK_NOT_DONE(2028, ""),
    ERR_SM_TIER_HYBRIDMOVE_ON_FLASH_VOLUME(2029, ""),
    ERR_SM_EXCEEDED_DISK_CAPACITY(2030, ""),
    ERR_SM_NOERR_GAINED_SM_TOKENS(2031, ""),
    ERR_SM_NOERR_LOST_SM_TOKENS(2032, ""),
    ERR_SM_NOERR_NEED_RESYNC(2033, ""),
    ERR_SM_NOERR_NOT_IN_DLT(2034, ""),
    ERR_SM_RESYNC_SOURCE_DECLINE(2035, ""),
    ERR_SM_NOT_READY_AS_MIGR_SRC(2036, ""),
    ERR_SM_NO_DISK(2037, ""),

    ERR_NETWORK_TRANSPORT(3000, ""),
    ERR_EP_NON_EXISTANT(3001, ""),

    ERR_FILE_DOES_NOT_EXIST(3100, ""),
    ERR_CHECKSUM_MISMATCH(3101, ""),

    ERR_SVC_REQUEST_FAILED(4000, ""),
    ERR_SVC_REQUEST_INVOCATION(4001, ""),
    ERR_SVC_REQUEST_USER_INTERRUPTED(4002, ""),
    ERR_SVC_REQUEST_TIMEOUT(4003, ""),
    ERR_SVC_SERVER_PORT_ALREADY_INUSE(4004, ""),
    ERR_SVC_SERVER_CRASH(4005, ""),

    FDSN_StatusCreated(5000, ""),
    FDSN_StatusNOTSET(5001, ""),
    FDSN_StatusInternalError(5002, ""),
    FDSN_StatusOutOfMemory(5003, ""),
    FDSN_StatusInterrupted(5004, ""),
    FDSN_StatusTxnInProgress(5005, ""),
    FDSN_StatusInvalidBucketNameTooLong(5006, ""),
    FDSN_StatusInvalidBucketNameFirstCharacter(5007, ""),
    FDSN_StatusInvalidBucketNameCharacter(5008, ""),
    FDSN_StatusInvalidBucketNameCharacterSequence(5009, ""),
    FDSN_StatusInvalidBucketNameTooShort(5010, ""),
    FDSN_StatusInvalidBucketNameDotQuadNotation(5011, ""),
    FDSN_StatusQueryParamsTooLong(5012, ""),
    FDSN_StatusFailedToInitializeRequest(5013, ""),
    FDSN_StatusMetaDataHeadersTooLong(5014, ""),
    FDSN_StatusBadMetaData(5015, ""),
    FDSN_StatusBadContentType(5016, ""),
    FDSN_StatusContentTypeTooLong(5017, ""),
    FDSN_StatusBadMD5(5018, ""),
    FDSN_StatusMD5TooLong(5019, ""),
    FDSN_StatusBadCacheControl(5020, ""),
    FDSN_StatusCacheControlTooLong(5021, ""),
    FDSN_StatusBadContentDispositionFilename(5022, ""),
    FDSN_StatusContentDispositionFilenameTooLong(5023, ""),
    FDSN_StatusBadContentEncoding(5024, ""),
    FDSN_StatusContentEncodingTooLong(5025, ""),
    FDSN_StatusBadIfMatchETag(5026, ""),
    FDSN_StatusIfMatchETagTooLong(5027, ""),
    FDSN_StatusBadIfNotMatchETag(5028, ""),
    FDSN_StatusIfNotMatchETagTooLong(5029, ""),
    FDSN_StatusHeadersTooLong(5030, ""),
    FDSN_StatusKeyTooLong(5031, ""),
    FDSN_StatusUriTooLong(5032, ""),
    FDSN_StatusXmlParseFailure(5033, ""),
    FDSN_StatusEmailAddressTooLong(5034, ""),
    FDSN_StatusUserIdTooLong(5035, ""),
    FDSN_StatusUserDisplayNameTooLong(5036, ""),
    FDSN_StatusGroupUriTooLong(5037, ""),
    FDSN_StatusPermissionTooLong(5038, ""),
    FDSN_StatusTargetBucketTooLong(5039, ""),
    FDSN_StatusTargetPrefixTooLong(5040, ""),
    FDSN_StatusTooManyGrants(5041, ""),
    FDSN_StatusBadGrantee(5042, ""),
    FDSN_StatusBadPermission(5043, ""),
    FDSN_StatusXmlDocumentTooLarge(5044, ""),
    FDSN_StatusNameLookupError(5045, ""),
    FDSN_StatusFailedToConnect(5046, ""),
    FDSN_StatusServerFailedVerification(5047, ""),
    FDSN_StatusConnectionFailed(5048, ""),
    FDSN_StatusAbortedByCallback(5049, ""),
    FDSN_StatusRequestTimedOut(5050, ""),
    FDSN_StatusEntityEmpty(5051, ""),
    FDSN_StatusEntityDoesNotExist(5052, ""),
    FDSN_StatusErrorAccessDenied(5053, ""),
    FDSN_StatusErrorAccountProblem(5054, ""),
    FDSN_StatusErrorAmbiguousGrantByEmailAddress(5055, ""),
    FDSN_StatusErrorBadDigest(5056, ""),
    FDSN_StatusErrorBucketAlreadyExists(5057, ""),
    FDSN_StatusErrorBucketNotExists(5058, ""),
    FDSN_StatusErrorBucketAlreadyOwnedByYou(5059, ""),
    FDSN_StatusErrorBucketNotEmpty(5060, ""),
    FDSN_StatusErrorCredentialsNotSupported(5061, ""),
    FDSN_StatusErrorCrossLocationLoggingProhibited(5062, ""),
    FDSN_StatusErrorEntityTooSmall(5063, ""),
    FDSN_StatusErrorEntityTooLarge(5064, ""),
    FDSN_StatusErrorMissingContentLength(5065, ""),

    PLATFORM_ERROR_UNEXPECTED_CHILD_DEATH(60000, ""),
    FDSN_StatusErrorUnknown(60001, ""),
    ERR_INVALID(60002, ""),
    ERR_MAX(60003, "");

    private final int errorCode;
    private final String message;
    private static Lazy<Map<Integer, FdsError>> errorMap = new Lazy<>(() -> createErrorMap());

    private static Map<Integer, FdsError> createErrorMap() {
        HashMap<Integer, FdsError> map = new HashMap<>();
        for(FdsError error : FdsError.values()) {
            map.put(error.errorCode, error);
        }
        return map;
    }

    FdsError(int errorCode, String message) {
        this.errorCode = errorCode;
        this.message = message;
    }

    public int getErrorCode() {
        return errorCode;
    }

    public String getMessage() {
        return message;
    }

    public static FdsError findByErrorCode(int key) {
        return errorMap.getInstance().getOrDefault(key, ERR_INVALID);
    }


    @Override
    public String toString() {
        if(message.equals("")) {
            return "[ERROR:" + errorCode + "] " + name();
        } else {
            return "[ERROR:" + errorCode + "] " + name() + ":" + message;
        }
    }
}

//
// ** FUN PYTHON SCRIPT TO DUMP THESE FROM fds_error.h **
//
// TODO: find me a home!  (or rewrite me in java and stuff me in here somewhere to be run in a test case!)
//
/* -- BEGIN SCRIPT --
#!/usr/bin/python

import re
import sys

if len(sys.argv) < 2:
    print "Usage: %s <PATH TO fds_error.h>" % (sys.argv[0])

target = sys.argv[1]

# ADD(ERR_OK,= 0,"ALL OK"), \
re_chunks = ["ADD", "\(", "(\w+)", ",", "=?", "(\d*)", ",", '"([^"]+)"', "\)"]
expr = re.compile("^\s*" + "\s*".join(re_chunks))

with open(target) as handle:
    lastNum = 0

    for line in handle:
        line = line.strip()
        match = expr.match(line)
        if line.startswith("ADD") and match is None:
            raise Exception("expecting to parse line but failed: " + line)

        if match is not None:
            name = match.group(1)
            number = match.group(2)
            if number is None or number == "":
                number = lastNum + 1
            else:
                number = int(number)

            if number > lastNum + 1:
                print

            lastNum = number

            string = match.group(3).strip()
            if string is None:
                string = ""

            print '    %s(%s, "%s"),' % (name, number, string)
-- END SCRIPT -- */
