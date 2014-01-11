#include <FDSP_ConfigPathReq.h>
#include <FDSP_constants.h>
#include <FDSP_ControlPathResp.h>
#include <FDSP_DataPathResp.h>
#include <FDSP_MetaDataPathResp.h>
#include <FDSP_types.h>
#include <FDSP_ConfigPathResp.h>
#include <FDSP_ControlPathReq.h>  
#include <FDSP_MetaDataPathReq.h>
#include <FDSP_SessionReq.h>
#include <FDSP_DataPathReq.h>

class fdspDataPathRespReceiver: public Runnable {
public:
  fdspDataPathRespReceiver(boost::shared_ptr<TProtocol> prot,
                   boost::shared_ptr<FDSP_DataPathRespIf> hdlr)
    : prot_(prot),
      processor_(new fdspDataPathRespProcessor(hdlr))
  {
  }

  void run() {
    /* wait for connection to be established */
    /* for now just busy wait */
    while (!prot_->getTransport()->isOpen()) {
      printf("fdspDataPathRespReceiver: waiting for transport...\n");
      usleep(500);
    }

    try {
      for (;;) {
        if (!processor_->process(prot_, prot_, NULL) ||
            !prot_->getTransport()->peek()) {
          break;
        }
      }
    }
    catch (TException& tx) {
      printf("fdspDataPathResponse Receiver exception");
    }
  }


public:
  boost::shared_ptr<TProtocol> prot_;
  boost::shared_ptr<TProcessor> processor_;
};

class fdspMetaDataPathRespReceiver: public Runnable {
public:
  fdspMetaDataPathRespReceiver(boost::shared_ptr<TProtocol> prot,
                   boost::shared_ptr<FDSP_MetaDataPathRespIf> hdlr)
    : prot_(prot),
      processor_(new fdspDataPathRespProcessor(hdlr))
  {
  }

  void run() {
    /* wait for connection to be established */
    /* for now just busy wait */
    while (!prot_->getTransport()->isOpen()) {
      printf("fdspMetaDataPathRespReceiver: waiting for transport...\n");
      usleep(500);
    }

    try {
      for (;;) {
        if (!processor_->process(prot_, prot_, NULL) ||
            !prot_->getTransport()->peek()) {
          break;
        }
      }
    }
    catch (TException& tx) {
      printf("fdspMetaDataPathResponse Receiver exception");
    }
  }


public:
  boost::shared_ptr<TProtocol> prot_;
  boost::shared_ptr<TProcessor> processor_;
};


class fdspControlPathRespReceiver: public Runnable {
public:
  fdspControlPathRespReceiver(boost::shared_ptr<TProtocol> prot,
                   boost::shared_ptr<FDSP_ControlPathRespIf> hdlr)
    : prot_(prot),
      processor_(new fdspControlPathRespProcessor(hdlr))
  {
  }

  void run() {
    /* wait for connection to be established */
    /* for now just busy wait */
    while (!prot_->getTransport()->isOpen()) {
      printf("fdspControlPathRespReceiver: waiting for transport...\n");
      usleep(500);
    }

    try {
      for (;;) {
        if (!processor_->process(prot_, prot_, NULL) ||
            !prot_->getTransport()->peek()) {
          break;
        }
      }
    }
    catch (TException& tx) {
      printf("fdspControlPathResponse Receiver exception");
    }
  }

public:
  boost::shared_ptr<TProtocol> prot_;
  boost::shared_ptr<TProcessor> processor_;
};

class fdspConfigPathRespReceiver: public Runnable {
public:
  fdspConfigPathRespReceiver(boost::shared_ptr<TProtocol> prot,
                   boost::shared_ptr<FDSP_ConfigPathRespIf> hdlr)
    : prot_(prot),
      processor_(new fdspConfigPathRespProcessor(hdlr))
  {
  }

  void run() {
    /* wait for connection to be established */
    /* for now just busy wait */
    while (!prot_->getTransport()->isOpen()) {
      printf("fdspConfigPathRespReceiver: waiting for transport...\n");
      usleep(500);
    }

    try {
      for (;;) {
        if (!processor_->process(prot_, prot_, NULL) ||
            !prot_->getTransport()->peek()) {
          break;
        }
      }
    }
    catch (TException& tx) {
      printf("fdspConfigPathResponse Receiver exception");
    }
  }

public:
  boost::shared_ptr<TProtocol> prot_;
  boost::shared_ptr<TProcessor> processor_;
};

