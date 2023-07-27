// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_FILEDATATABLE_H_
#define FLATBUFFERS_GENERATED_FILEDATATABLE_H_

#include "flatbuffers/flatbuffers.h"

// Ensure the included flatbuffers.h is the same version as when this file was
// generated, otherwise it may not be compatible.
static_assert(FLATBUFFERS_VERSION_MAJOR == 23 &&
              FLATBUFFERS_VERSION_MINOR == 5 &&
              FLATBUFFERS_VERSION_REVISION == 26,
             "Non-compatible flatbuffers version included");

struct PacketHeader;

struct FileDataDefinition;
struct FileDataDefinitionBuilder;

struct FileDataTable;
struct FileDataTableBuilder;

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) PacketHeader FLATBUFFERS_FINAL_CLASS {
 private:
  int32_t StreamID_;
  int32_t Size_;

 public:
  PacketHeader()
      : StreamID_(0),
        Size_(0) {
  }
  PacketHeader(int32_t _StreamID, int32_t _Size)
      : StreamID_(::flatbuffers::EndianScalar(_StreamID)),
        Size_(::flatbuffers::EndianScalar(_Size)) {
  }
  int32_t StreamID() const {
    return ::flatbuffers::EndianScalar(StreamID_);
  }
  int32_t Size() const {
    return ::flatbuffers::EndianScalar(Size_);
  }
};
FLATBUFFERS_STRUCT_END(PacketHeader, 8);

struct FileDataDefinition FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef FileDataDefinitionBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_BYTEOFFSET = 4,
    VT_PACKETINFO = 6,
    VT_NUMELEMENTS = 8,
    VT_TIMESTAMPSTART = 10,
    VT_TIMESTAMPEND = 12
  };
  int64_t ByteOffset() const {
    return GetField<int64_t>(VT_BYTEOFFSET, 0);
  }
  const PacketHeader *PacketInfo() const {
    return GetStruct<const PacketHeader *>(VT_PACKETINFO);
  }
  int64_t NumElements() const {
    return GetField<int64_t>(VT_NUMELEMENTS, 0);
  }
  int64_t TimestampStart() const {
    return GetField<int64_t>(VT_TIMESTAMPSTART, 0);
  }
  int64_t TimestampEnd() const {
    return GetField<int64_t>(VT_TIMESTAMPEND, 0);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int64_t>(verifier, VT_BYTEOFFSET, 8) &&
           VerifyField<PacketHeader>(verifier, VT_PACKETINFO, 4) &&
           VerifyField<int64_t>(verifier, VT_NUMELEMENTS, 8) &&
           VerifyField<int64_t>(verifier, VT_TIMESTAMPSTART, 8) &&
           VerifyField<int64_t>(verifier, VT_TIMESTAMPEND, 8) &&
           verifier.EndTable();
  }
};

struct FileDataDefinitionBuilder {
  typedef FileDataDefinition Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_ByteOffset(int64_t ByteOffset) {
    fbb_.AddElement<int64_t>(FileDataDefinition::VT_BYTEOFFSET, ByteOffset, 0);
  }
  void add_PacketInfo(const PacketHeader *PacketInfo) {
    fbb_.AddStruct(FileDataDefinition::VT_PACKETINFO, PacketInfo);
  }
  void add_NumElements(int64_t NumElements) {
    fbb_.AddElement<int64_t>(FileDataDefinition::VT_NUMELEMENTS, NumElements, 0);
  }
  void add_TimestampStart(int64_t TimestampStart) {
    fbb_.AddElement<int64_t>(FileDataDefinition::VT_TIMESTAMPSTART, TimestampStart, 0);
  }
  void add_TimestampEnd(int64_t TimestampEnd) {
    fbb_.AddElement<int64_t>(FileDataDefinition::VT_TIMESTAMPEND, TimestampEnd, 0);
  }
  explicit FileDataDefinitionBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<FileDataDefinition> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<FileDataDefinition>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<FileDataDefinition> CreateFileDataDefinition(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    int64_t ByteOffset = 0,
    const PacketHeader *PacketInfo = nullptr,
    int64_t NumElements = 0,
    int64_t TimestampStart = 0,
    int64_t TimestampEnd = 0) {
  FileDataDefinitionBuilder builder_(_fbb);
  builder_.add_TimestampEnd(TimestampEnd);
  builder_.add_TimestampStart(TimestampStart);
  builder_.add_NumElements(NumElements);
  builder_.add_ByteOffset(ByteOffset);
  builder_.add_PacketInfo(PacketInfo);
  return builder_.Finish();
}

struct FileDataTable FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef FileDataTableBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_TABLE = 4
  };
  const ::flatbuffers::Vector<::flatbuffers::Offset<FileDataDefinition>> *Table() const {
    return GetPointer<const ::flatbuffers::Vector<::flatbuffers::Offset<FileDataDefinition>> *>(VT_TABLE);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_TABLE) &&
           verifier.VerifyVector(Table()) &&
           verifier.VerifyVectorOfTables(Table()) &&
           verifier.EndTable();
  }
};

struct FileDataTableBuilder {
  typedef FileDataTable Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_Table(::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<FileDataDefinition>>> Table) {
    fbb_.AddOffset(FileDataTable::VT_TABLE, Table);
  }
  explicit FileDataTableBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<FileDataTable> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<FileDataTable>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<FileDataTable> CreateFileDataTable(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    ::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<FileDataDefinition>>> Table = 0) {
  FileDataTableBuilder builder_(_fbb);
  builder_.add_Table(Table);
  return builder_.Finish();
}

inline ::flatbuffers::Offset<FileDataTable> CreateFileDataTableDirect(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    const std::vector<::flatbuffers::Offset<FileDataDefinition>> *Table = nullptr) {
  auto Table__ = Table ? _fbb.CreateVector<::flatbuffers::Offset<FileDataDefinition>>(*Table) : 0;
  return CreateFileDataTable(
      _fbb,
      Table__);
}

inline const FileDataTable *GetFileDataTable(const void *buf) {
  return ::flatbuffers::GetRoot<FileDataTable>(buf);
}

inline const FileDataTable *GetSizePrefixedFileDataTable(const void *buf) {
  return ::flatbuffers::GetSizePrefixedRoot<FileDataTable>(buf);
}

inline const char *FileDataTableIdentifier() {
  return "FTAB";
}

inline bool FileDataTableBufferHasIdentifier(const void *buf) {
  return ::flatbuffers::BufferHasIdentifier(
      buf, FileDataTableIdentifier());
}

inline bool SizePrefixedFileDataTableBufferHasIdentifier(const void *buf) {
  return ::flatbuffers::BufferHasIdentifier(
      buf, FileDataTableIdentifier(), true);
}

inline bool VerifyFileDataTableBuffer(
    ::flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<FileDataTable>(FileDataTableIdentifier());
}

inline bool VerifySizePrefixedFileDataTableBuffer(
    ::flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<FileDataTable>(FileDataTableIdentifier());
}

inline void FinishFileDataTableBuffer(
    ::flatbuffers::FlatBufferBuilder &fbb,
    ::flatbuffers::Offset<FileDataTable> root) {
  fbb.Finish(root, FileDataTableIdentifier());
}

inline void FinishSizePrefixedFileDataTableBuffer(
    ::flatbuffers::FlatBufferBuilder &fbb,
    ::flatbuffers::Offset<FileDataTable> root) {
  fbb.FinishSizePrefixed(root, FileDataTableIdentifier());
}

#endif  // FLATBUFFERS_GENERATED_FILEDATATABLE_H_
