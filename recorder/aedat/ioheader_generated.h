// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_IOHEADER_H_
#define FLATBUFFERS_GENERATED_IOHEADER_H_

#include "flatbuffers/flatbuffers.h"

// Ensure the included flatbuffers.h is the same version as when this file was
// generated, otherwise it may not be compatible.
static_assert(FLATBUFFERS_VERSION_MAJOR == 23 &&
              FLATBUFFERS_VERSION_MINOR == 5 &&
              FLATBUFFERS_VERSION_REVISION == 26,
             "Non-compatible flatbuffers version included");


struct IOHeader;
struct IOHeaderBuilder;

enum Constants : int32_t {
  Constants_AEDAT_VERSION_LENGTH = 14,
  Constants_MIN = Constants_AEDAT_VERSION_LENGTH,
  Constants_MAX = Constants_AEDAT_VERSION_LENGTH
};

inline const Constants (&EnumValuesConstants())[1] {
  static const Constants values[] = {
    Constants_AEDAT_VERSION_LENGTH
  };
  return values;
}

inline const char * const *EnumNamesConstants() {
  static const char * const names[2] = {
    "AEDAT_VERSION_LENGTH",
    nullptr
  };
  return names;
}

inline const char *EnumNameConstants(Constants e) {
  if (::flatbuffers::IsOutRange(e, Constants_AEDAT_VERSION_LENGTH, Constants_AEDAT_VERSION_LENGTH)) return "";
  const size_t index = static_cast<size_t>(e) - static_cast<size_t>(Constants_AEDAT_VERSION_LENGTH);
  return EnumNamesConstants()[index];
}

enum CompressionType : int32_t {
  CompressionType_NONE = 0,
  CompressionType_LZ4 = 1,
  CompressionType_LZ4_HIGH = 2,
  CompressionType_ZSTD = 3,
  CompressionType_ZSTD_HIGH = 4,
  CompressionType_MIN = CompressionType_NONE,
  CompressionType_MAX = CompressionType_ZSTD_HIGH
};

inline const CompressionType (&EnumValuesCompressionType())[5] {
  static const CompressionType values[] = {
    CompressionType_NONE,
    CompressionType_LZ4,
    CompressionType_LZ4_HIGH,
    CompressionType_ZSTD,
    CompressionType_ZSTD_HIGH
  };
  return values;
}

inline const char * const *EnumNamesCompressionType() {
  static const char * const names[6] = {
    "NONE",
    "LZ4",
    "LZ4_HIGH",
    "ZSTD",
    "ZSTD_HIGH",
    nullptr
  };
  return names;
}

inline const char *EnumNameCompressionType(CompressionType e) {
  if (::flatbuffers::IsOutRange(e, CompressionType_NONE, CompressionType_ZSTD_HIGH)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesCompressionType()[index];
}

struct IOHeader FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef IOHeaderBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_COMPRESSION = 4,
    VT_DATATABLEPOSITION = 6,
    VT_INFONODE = 8
  };
  CompressionType compression() const {
    return static_cast<CompressionType>(GetField<int32_t>(VT_COMPRESSION, 0));
  }
  int64_t dataTablePosition() const {
    return GetField<int64_t>(VT_DATATABLEPOSITION, -1LL);
  }
  const ::flatbuffers::String *infoNode() const {
    return GetPointer<const ::flatbuffers::String *>(VT_INFONODE);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int32_t>(verifier, VT_COMPRESSION, 4) &&
           VerifyField<int64_t>(verifier, VT_DATATABLEPOSITION, 8) &&
           VerifyOffset(verifier, VT_INFONODE) &&
           verifier.VerifyString(infoNode()) &&
           verifier.EndTable();
  }
};

struct IOHeaderBuilder {
  typedef IOHeader Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_compression(CompressionType compression) {
    fbb_.AddElement<int32_t>(IOHeader::VT_COMPRESSION, static_cast<int32_t>(compression), 0);
  }
  void add_dataTablePosition(int64_t dataTablePosition) {
    fbb_.AddElement<int64_t>(IOHeader::VT_DATATABLEPOSITION, dataTablePosition, -1LL);
  }
  void add_infoNode(::flatbuffers::Offset<::flatbuffers::String> infoNode) {
    fbb_.AddOffset(IOHeader::VT_INFONODE, infoNode);
  }
  explicit IOHeaderBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<IOHeader> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<IOHeader>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<IOHeader> CreateIOHeader(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    CompressionType compression = CompressionType_NONE,
    int64_t dataTablePosition = -1LL,
    ::flatbuffers::Offset<::flatbuffers::String> infoNode = 0) {
  IOHeaderBuilder builder_(_fbb);
  builder_.add_dataTablePosition(dataTablePosition);
  builder_.add_infoNode(infoNode);
  builder_.add_compression(compression);
  return builder_.Finish();
}

inline ::flatbuffers::Offset<IOHeader> CreateIOHeaderDirect(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    CompressionType compression = CompressionType_NONE,
    int64_t dataTablePosition = -1LL,
    const char *infoNode = nullptr) {
  auto infoNode__ = infoNode ? _fbb.CreateString(infoNode) : 0;
  return CreateIOHeader(
      _fbb,
      compression,
      dataTablePosition,
      infoNode__);
}

inline const IOHeader *GetIOHeader(const void *buf) {
  return ::flatbuffers::GetRoot<IOHeader>(buf);
}

inline const IOHeader *GetSizePrefixedIOHeader(const void *buf) {
  return ::flatbuffers::GetSizePrefixedRoot<IOHeader>(buf);
}

inline const char *IOHeaderIdentifier() {
  return "IOHE";
}

inline bool IOHeaderBufferHasIdentifier(const void *buf) {
  return ::flatbuffers::BufferHasIdentifier(
      buf, IOHeaderIdentifier());
}

inline bool SizePrefixedIOHeaderBufferHasIdentifier(const void *buf) {
  return ::flatbuffers::BufferHasIdentifier(
      buf, IOHeaderIdentifier(), true);
}

inline bool VerifyIOHeaderBuffer(
    ::flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<IOHeader>(IOHeaderIdentifier());
}

inline bool VerifySizePrefixedIOHeaderBuffer(
    ::flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<IOHeader>(IOHeaderIdentifier());
}

inline void FinishIOHeaderBuffer(
    ::flatbuffers::FlatBufferBuilder &fbb,
    ::flatbuffers::Offset<IOHeader> root) {
  fbb.Finish(root, IOHeaderIdentifier());
}

inline void FinishSizePrefixedIOHeaderBuffer(
    ::flatbuffers::FlatBufferBuilder &fbb,
    ::flatbuffers::Offset<IOHeader> root) {
  fbb.FinishSizePrefixed(root, IOHeaderIdentifier());
}

#endif  // FLATBUFFERS_GENERATED_IOHEADER_H_
