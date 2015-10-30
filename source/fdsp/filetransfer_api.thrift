namespace c_glib FDS_ProtocolInterface
namespace cpp FDS_ProtocolInterface
namespace java com.formationds.protocol

/**
 * Message to send a file in chunks
 */ 
struct FileTransferMsg {
  1: string filename,
  2: string data,
  3: i64 offset
}

/**
 * Message to verify file's checksum after transfer
 */
struct FileTransferVerifyMsg {
  1: string filename,
  2: string checksum
}