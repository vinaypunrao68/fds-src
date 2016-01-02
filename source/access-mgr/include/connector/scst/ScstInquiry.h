/*
 * scst/ScstInquiry.h
 *
 * Copyright (c) 2015, Brian Szmyd <szmyd@formationds.com>
 * Copyright (c) 2015, Formation Data Systems
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTINQUIRY_H_
#define SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTINQUIRY_H_

#include <cstdint>
#include <cstring>
extern "C" {
#include <endian.h>
}
#include <map>
#include <string>
#include <type_traits>
#include <vector>

namespace fds { 

struct ScstTask;

template<typename E>
constexpr auto to_underlying(E e) -> typename std::underlying_type<E>::type 
{
   return static_cast<typename std::underlying_type<E>::type>(e);
}

enum struct PeripheralQualifer : uint8_t
{
    ConnectedOrUnknown      = 0b000,
    NotConnectedButCapable  = 0b001,
    NotCapable              = 0b011,
};

enum struct PeripheralType : uint8_t
{
    DirectAccess        = 0x00,
    SequentialAccess    = 0x01,
    Printer             = 0x02,
    Processor           = 0x03,
    WriteOnce           = 0x04,
    CD_DVD              = 0x05,
    OpticalMemory       = 0x07,
    MediumChanger       = 0x08,
    StorageArray        = 0x0C,
    EnclosureService    = 0x0D,
    SimplifiedDisk      = 0x0E,
    OpticalCard         = 0x0F,
    ObjectStore         = 0x11,
    AutomationInterface = 0x12,
};

#ifdef __LITTLE_ENDIAN
/// Bit-fields in little endian GCC are backwards
struct __attribute__((__packed__)) PeripheralId {
    uint8_t _device_type : 5, _qualifier : 3;
};

struct __attribute__((__packed__)) StdInqHeader {
    StdInqHeader() { std::memset(this, '\0', sizeof(StdInqHeader)); }
    PeripheralId _peripheral;
    uint8_t : 6, _rmb : 1, _lu_cong: 1;
    uint8_t _version;
    uint8_t _response_format : 4, _hisup: 1, _normaca : 1, : 0;
    uint8_t _additional_length;
    uint8_t _protect : 1, : 2, _3pc : 1, _tpgs : 2, _acc : 1, _sccs : 1;
    uint8_t _addr16 : 1, : 3, _multip : 1, : 1, _encserv : 1, : 0;
    uint8_t : 1, _cmdque: 1, : 2, _sync : 1, _wbus16 : 1, : 0;
};
static_assert(8 == sizeof(StdInqHeader), "Size of StdInqHeader has changed!");

struct __attribute__((__packed__)) VPDHeader {
    VPDHeader() { std::memset(this, '\0', sizeof(VPDHeader)); }

    PeripheralId _peripheral;
    uint8_t _page_code;
    uint16_t _page_length;
};
static_assert(4 == sizeof(VPDHeader), "Size of VPDHeader has changed!");

struct __attribute__((__packed__)) ExtVPDParameters {
    enum HeadSup : bool { NoHeadAttrSupport, HeadAttrSupport };
    enum OrdSup : bool { NoOrderedAttrSupport, OrderedAttrSupport };
    enum SimpSup : bool { NoSimpleAttrSupport, SimpleAttrSupport };

    ExtVPDParameters() { std::memset(this, '\0', sizeof(ExtVPDParameters)); }

    void operator &=(HeadSup const head_support) { _headsup = to_underlying(head_support); }
    void operator &=(OrdSup const ord_support) { _ordsup = to_underlying(ord_support); }
    void operator &=(SimpSup const simp_support) { _simpsup = to_underlying(simp_support); }

 private:
    uint8_t _ref_chk : 1, _app_chk : 1, _grd_chk : 1, _spt : 3, _active_microcode : 2;
    uint8_t _simpsup : 1, _ordsup : 1, _headsup : 1, _priorsup : 1, _groupsup : 1, _uasksup : 1, : 0;
    uint8_t _vsup : 1, _nvsup : 1, _crdsup : 1, _wusup : 1, : 0;
    uint8_t _luiclr : 1, : 3, _piisup : 1, : 0;
    uint8_t _cbcs : 1, : 3, _rsup : 1, : 0;
    uint8_t _multi_nexus_download : 4, : 0;
    uint16_t _extended_selftest_completion;
    uint8_t : 5, _vsasup : 1, _hrasup : 1, _poasup : 1;
    uint8_t _maximum_supported_sense_length;
    uint8_t _reserved[50];
};
static_assert(60 == sizeof(ExtVPDParameters), "Size of ExtVPDParameters has changed!");

struct __attribute__((__packed__)) DesignatorHeader {
    enum Assoc : uint8_t { LUNAssociation, PortAssociation, TargetAssociation };
    enum CodeSet : uint8_t { BinaryCodeSet = 1, ASCIICodeSet, UTF8CodeSet };
    enum Type : uint8_t {
        VendorSpecific,
        T10VendorId,
        EUI64,
        NAA,
        RelativePortId,
        TargetPortGroup,
        LogicalUnitGroup,
        MD5Id,
        SCSIName,
        ProtocolSpecific,
    };

    DesignatorHeader() { std::memset(this, '\0', sizeof(DesignatorHeader)); }

    void operator &=(Assoc const association) { _association = to_underlying(association); }
    void operator &=(CodeSet const codeset) { _code_set = to_underlying(codeset); }
    void operator &=(Type const type) { _designator_type = to_underlying(type); }

    uint8_t _code_set : 4, _protocol_identifier : 4;
    uint8_t _designator_type : 4, _association : 2, : 1, _piv : 1;
    uint8_t _reserved;
    uint8_t _designator_length;
};

#else
#error "Big Endian not supported!"
#endif

struct __attribute__((__packed__)) VPDPage {
  uint8_t getPageCode() const { return _header._page_code; }
  size_t getParamLength() const { return be16toh(_header._page_length); }
  void writePage(uint8_t const page_code, void const* params, size_t const len);

  void operator &=(PeripheralQualifer const qualifier) { _header._peripheral._qualifier = to_underlying(qualifier); }
  void operator &=(PeripheralType const device_type) { _header._peripheral._device_type = to_underlying(device_type); }

 private:
  VPDHeader _header;
  uint8_t _parameters[60];
};
static_assert(64 == sizeof(VPDPage), "Size of VPDPage has changed!");

struct __attribute__((__packed__)) NAADesignator {
  NAADesignator();
  NAADesignator(uint32_t const company_id, uint64_t const vendor_id); 

 private:
  DesignatorHeader _header;
  uint8_t _hi_company_id : 4, _naa : 4;
  uint16_t _mid_company_id;
  uint8_t _hi_vendor_id : 4, _low_company_id : 4;
  uint32_t _low_vendor_id;
};
static_assert(12 == sizeof(NAADesignator), "Size of NAADesignator has changed!");

struct __attribute__((__packed__)) T10Designator {
  T10Designator();
  explicit T10Designator(std::string const& vendor_id);

 private:
  DesignatorHeader _header;
  uint8_t _t10_vendor_id[8];
};
static_assert(12 == sizeof(T10Designator), "Size of T10Designator has changed!");

struct __attribute__((__packed__)) VendorSpecificIdentifier {
  VendorSpecificIdentifier();
  explicit VendorSpecificIdentifier(std::string const& id);

 private:
  DesignatorHeader _header;
  uint8_t _id[32] {};
};
static_assert(36 == sizeof(VendorSpecificIdentifier), "Size of VendorSpecificIdentifier has changed!");

struct DescriptorBuilder {
  template<typename Desc>
  void operator &=(Desc && descriptor);
  uint8_t const* data() const { return _descriptor_list.data(); }
  size_t length() const { return _descriptor_list.size(); }

 private:
  std::vector<uint8_t> _descriptor_list;
};

template<typename Desc>
void DescriptorBuilder::operator &=(Desc && descriptor)
{
    static_assert(4 < sizeof(Desc), "Refusing to add designators smaller than 4 bytes.");
    auto raw_descriptor = reinterpret_cast<uint8_t*>(&descriptor);
    size_t length = raw_descriptor[3];
    if ((sizeof(descriptor) - 4) < length ) return; // Refuse to read into out-of-bounds memory
    auto old_length = _descriptor_list.size();
    _descriptor_list.resize(old_length + sizeof(descriptor));
    memcpy(&_descriptor_list[old_length], &descriptor, sizeof(descriptor));
}

struct __attribute__((__packed__)) StandardInquiry {
    enum ACC : bool { NoAccessController, ContainsAccessController };
    enum CmdQue : bool { NoCommandMgmt, CommandMgmt };
    enum EncServ : bool { NoEnclosure, ContainsEnclosure };
    enum HiSup : bool { NoHistoricalSupport, HistoricalSupport };
    enum LUCong : bool { NotPartOfConglomerate, PartOfConglomerate };
    enum MultiP : bool { NotMultiPorted, MultiPorted };
    enum NormACA : bool { NoNACASupport, NACASupport };
    enum Protect : bool { NoProtection, ProtectionSupported };
    enum RMB : bool { NotRemovable, Removable };
    enum SCCS : bool { NoArrayController, ContainsArrayController };
    enum TPC : bool { NoThirdPartyCopy, ThirdPartyCopy };
    enum TPGS : uint8_t {
        NoAsymmetricSupport, ImplicitAsymmetricSupport, ExplicitAsymmetricSupport, FullAsymmetricSupport
    };

    StandardInquiry();

    uint8_t rf() const { return _header._response_format; }

    void operator &=(ACC const access_controller) { _header._acc = to_underlying(access_controller); }
    void operator &=(CmdQue const command_management) { _header._cmdque = to_underlying(command_management); }
    void operator &=(EncServ const enclosure_service) { _header._encserv = to_underlying(enclosure_service); }
    void operator &=(HiSup const hist_support) { _header._hisup = to_underlying(hist_support); }
    void operator &=(LUCong const lu_conglomerate) { _header._lu_cong = to_underlying(lu_conglomerate); }
    void operator &=(NormACA const normal_aca) { _header._normaca = to_underlying(normal_aca); }
    void operator &=(MultiP const multi_ported) { _header._multip = to_underlying(multi_ported); }
    void operator &=(PeripheralQualifer const qualifier) { _header._peripheral._qualifier = to_underlying(qualifier); }
    void operator &=(PeripheralType const device_type) { _header._peripheral._device_type = to_underlying(device_type); }
    void operator &=(Protect const protection) { _header._protect = to_underlying(protection); }
    void operator &=(RMB const removable) { _header._rmb = to_underlying(removable); }
    void operator &=(SCCS const array_contoller) { _header._sccs = to_underlying(array_contoller); }
    void operator &=(TPC const third_party_copy) { _header._3pc = to_underlying(third_party_copy); }
    void operator &=(TPGS const asymmetric_access) { _header._tpgs = to_underlying(asymmetric_access); }

    bool operator &(HiSup const hist_support) const { return to_underlying(hist_support) == _header._hisup; }
    bool operator &(PeripheralQualifer const qualifier) const { return to_underlying(qualifier) == _header._peripheral._qualifier; }

    void setProductId(std::string const & product_id);
    void setRevision(std::string const & revision);
    void setVendorId(std::string const & vendor_id);

 private:
  static constexpr size_t t10_vendor_length = 8;
  static constexpr size_t product_id_length = 16;
  static constexpr size_t product_rev_length = 4;

  StdInqHeader _header;

  char _t10_vendor_id[t10_vendor_length];
  char _product_identification[product_id_length];
  char _product_revision_level[product_rev_length];
};
static_assert(36 == sizeof(StandardInquiry), "Size of StandardInquiry has changed!");

struct InquiryHandler {
  InquiryHandler();

  StandardInquiry getStandardInquiry() const { return standard_inquiry; }
  void setStandardInquiry(StandardInquiry const inquiry) { standard_inquiry = inquiry; }
  void writeStandardInquiry(ScstTask* task);

  void addVPDPage(VPDPage page);
  void writeVPDPage(ScstTask* task, uint8_t const page_code);

 private:
  void writeToBuffer(ScstTask* task, void* src, size_t const len);

  StandardInquiry standard_inquiry;
  std::map<uint8_t, VPDPage> vpd_pages;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTINQUIRY_H_
