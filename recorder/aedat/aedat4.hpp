#pragma once

#include <atomic>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdlib.h>
#include <unistd.h>
#include <vector>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <lz4.h>
#include <lz4frame.h>

#include <flatbuffers/flatbuffers.h>

#include "utils.hpp"
#include "aer.hpp"

#include "aedat.hpp"
#include "events_generated.h"
#include "file_data_table_generated.h"
#include "frame_generated.h"
// #include "generator.hpp"
#include "imus_generated.h"
#include "ioheader_generated.h"
#include "rapidxml.hpp"
#include "trigger_generated.h"

struct AEDAT4 : FileBase {

  struct Frame {
    int64_t time;
    int16_t width;
    int16_t height;
    std::vector<uint8_t> pixels;
  };

  struct OutInfo {
    enum Type { EVTS, FRME, IMUS, TRIG };
    int name;
    int size_x;
    int size_y;
    Type type;
    std::string compression;

    static Type to_type(std::string str) {
      if (str == "EVTS") {
        return Type::EVTS;
      } else if (str == "FRME") {
        return Type::FRME;
      } else if (str == "IMUS") {
        return Type::IMUS;
      } else if (str == "TRIG") {
        return Type::TRIG;
      } else {
        throw std::runtime_error("unexpected event type");
      }
    }
  };

  static std::tuple<char *, size_t> compress_lz4(char *buffer, size_t size) {
    auto lz4FrameBound = LZ4F_compressBound(size, nullptr);
    auto new_buffer = new char[lz4FrameBound];

    auto compression =
        LZ4F_compressFrame(new_buffer, lz4FrameBound, buffer, size, nullptr);
    return {new_buffer, compression};
    // LZ4F_compressionContext_t ctx;
    // auto contextCreation = LZ4F_createCompressionContext(&ctx,
    // LZ4F_VERSION);
    // auto headerSize =
    //     LZ4F_compressBegin(ctx, new_buffer, lz4FrameBound, nullptr);
    // if (LZ4F_isError(headerSize)) {
    //   std::stringstream err;
    //   err << "Compression error: " << LZ4F_getErrorName(headerSize);
    //   throw std::runtime_error(err.str());
    // }

    // auto packetSize = LZ4F_compressUpdate(ctx, new_buffer, lz4FrameBound,
    //                                       buffer, size, nullptr);
    // auto footerSize = LZ4F_compressEnd(ctx, new_buffer, lz4FrameBound,
    // nullptr);
    // return {new_buffer, headerSize + packetSize + footerSize};
  }

  static size_t save_header(std::fstream &stream) {
    stream << "#!AER-DAT4.0\r\n";
    // Save header
    auto infoNode =
R"""(<dv version="2.0">
    <node name="outInfo" path="/mainloop/Recorder/outInfo/">
        <node name="0" path="/mainloop/Recorder/outInfo/0/">
            <attr key="compression" type="string">LZ4</attr>
            <attr key="originalModuleName" type="string">capture</attr>
            <attr key="originalOutputName" type="string">events</attr>
            <attr key="typeDescription" type="string">Array of events (polarity ON/OFF).</attr>
            <attr key="typeIdentifier" type="string">EVTS</attr>
            <node name="info" path="/mainloop/Recorder/outInfo/0/info/">
                <attr key="sizeX" type="int">1280</attr>
                <attr key="sizeY" type="int">720</attr>
                <attr key="source" type="string">Prophesee EVK3.1</attr>
                <attr key="tsOffset" type="long">0</attr>
            </node>
        </node>
    </node>
</dv>)""";

    flatbuffers::FlatBufferBuilder fbb{2 * 1024};
    fbb.ForceDefaults(true);
    auto headerOffset =
        CreateIOHeaderDirect(fbb, CompressionType_LZ4, -1L, infoNode);

    FinishSizePrefixedIOHeaderBuffer(fbb, headerOffset);

    stream.write(reinterpret_cast<char *>(fbb.GetBufferPointer()), static_cast<std::streamsize>(fbb.GetSize()));
    std::cout << "Data " << stream.tellp() << " Header " << fbb.GetSize() << std::endl;
    return fbb.GetSize();
  }

  static void save_footer(std::fstream &stream, size_t headerSize,
                          int64_t timestampStart, int64_t timestampEnd,
                          size_t eventCount) {
    flatbuffers::FlatBufferBuilder fbb;
    // Mutate offset to data table
    size_t tableOffset = stream.tellp();
    stream.seekg(14); // 14 bytes for version
    auto length = headerSize;
    char *data = new char[length];
    stream.read(data, length);
    auto header = GetSizePrefixedIOHeader(data);
    auto new_header = CreateIOHeaderDirect(
        fbb, header->compression(), tableOffset, header->infoNode()->c_str());
    FinishSizePrefixedIOHeaderBuffer(fbb, new_header);
    stream.seekp(14); // 14 bytes for version
    stream.write((char *)fbb.GetBufferPointer(), fbb.GetSize());
    stream.seekp(tableOffset);

    // Write data table
    fbb.Clear();
    auto defBuilder = FileDataDefinitionBuilder(fbb);
    auto packetHeader = PacketHeader(0, eventCount);
    defBuilder.add_PacketInfo(&packetHeader);
    defBuilder.add_NumElements(1);
    defBuilder.add_TimestampStart(timestampStart);
    defBuilder.add_TimestampEnd(timestampEnd);
    auto dataDefinition = defBuilder.Finish();
    auto tables = std::vector{dataDefinition};
    auto tableVector = fbb.CreateVector(tables);
    auto dataTable = CreateFileDataTable(fbb, tableVector);
    FinishSizePrefixedFileDataTableBuffer(fbb, dataTable);
    auto [compressed, size] =
        compress_lz4((char *)fbb.GetBufferPointer(), fbb.GetSize());
    stream.write(compressed, size);
    std::cout << "Table " << tableOffset << std::endl;
    delete[] data;
    delete[] compressed;
  }

  static void save_events(std::fstream &stream,
                          std::vector<AEDAT::PolarityEvent> events) {
    // Create event buffer
    flatbuffers::FlatBufferBuilder fbb;
    fbb.ForceDefaults(true);
    std::vector<Event> bufferEvents;
    for (auto event : events) {
      const Event e = Event(static_cast<int64_t>(event.timestamp),
                            static_cast<int16_t>(event.x),
                            static_cast<int16_t>(event.y), event.polarity);
      bufferEvents.push_back(e);
    }
    auto eventVector = CreateEventPacketDirect(fbb, &bufferEvents);
    FinishSizePrefixedEventPacketBuffer(fbb, eventVector);
    auto [compressed, size] =
        compress_lz4((char *)fbb.GetBufferPointer(), fbb.GetSize());

    // Write packet header
    auto packetHeader = PacketHeader(0, size);
    stream.write((char *)&packetHeader, 8);

    // Write events
    stream.write(compressed, size);
    delete[] compressed;
  }

  std::tuple<AER::Event *, size_t> read_events(const int64_t n_events = -1) {
    int64_t byte_count =
        (n_events > 0 ? n_events : total_number_of_events) * sizeof(AER::Event);
    AER::Event *events = (AER::Event *)malloc(byte_count);

    int64_t count = 0;
    while (get_packet()) {
      for (; packet_events_read < event_vector->size(); ++packet_events_read) {
        const Event *event = event_vector->Get(packet_events_read);
        if (n_events > 0 && n_events - count <= 0) {
          return {events, count};
        }

        const auto e = AER::Event{
            static_cast<uint64_t>(event->t()),
            static_cast<uint16_t>(event->x()),
            static_cast<uint16_t>(event->y()),
            static_cast<bool>(event->on()),
        };
        events[count] = e;
        count += 1;
      }
    }
    return {events, count};
  }

  explicit AEDAT4(file_t &&fp)
      : fp{std::move(fp)}, dst_buffer(4096), packet_buffer(4096) {
    read_file_header();
  }
  explicit AEDAT4(const std::string &filename) : AEDAT4(open_file(filename)) {}
  ~AEDAT4() {
    if (ctx) {
      LZ4F_freeDecompressionContext(ctx);
    }
  }

private:
  LZ4F_decompressionContext_t ctx;
  const file_t fp;

  size_t total_number_of_events = 0;
  std::vector<uint8_t> dst_buffer;
  std::vector<char> packet_buffer;
  size_t packet_index = 0;
  size_t packet_events_read = 0;
  std::vector<size_t> event_packet_offsets;
  const flatbuffers::Vector<const Event *> *event_vector = nullptr;

  std::map<std::string, std::string>
  collect_attributes(rapidxml::xml_node<> *node) {
    std::map<std::string, std::string> attributes;
    for (const rapidxml::xml_attribute<> *a = node->first_attribute(); a;
         a = a->next_attribute()) {
      auto name = std::string(a->name(), a->name_size());
      auto value = std::string(a->value(), a->value_size());
      attributes[name] = value;
    }
    return attributes;
  }

  void read_file_header() {
    static const uint32_t VERSION_STRING_SIZE = 14;
    std::vector<char> data_buffer(BUFFER_SIZE);

    struct stat stat_info;
    if (fstat(fileno(fp.get()), &stat_info)) {
      throw std::runtime_error("Failed to stat file");
    }

    auto header_ret = fread(data_buffer.data(), 1, dst_buffer.size(), fp.get());
    if (header_ret <= 0) {
      throw std::runtime_error("Failed to read file version number");
    }

    auto header = std::string(data_buffer.data(), VERSION_STRING_SIZE);
    if (header != "#!AER-DAT4.0\r\n") {
      throw std::runtime_error("Invalid AEDAT version");
    }

    // find size of IOHeader (it is variable)
    flatbuffers::uoffset_t ioheader_offset =
        *reinterpret_cast<flatbuffers::uoffset_t *>(data_buffer.data() + VERSION_STRING_SIZE);
    const IOHeader *ioheader =
        GetSizePrefixedIOHeader(data_buffer.data() + VERSION_STRING_SIZE);

    // TODO: We currently only support LZ4 compression
    if (ioheader->compression() != CompressionType_LZ4 &&
        ioheader->compression() != CompressionType_LZ4_HIGH) {
      throw std::runtime_error("Only LZ4 compression is supported");
    }

    const size_t data_table_position = ioheader->dataTablePosition();
    if (data_table_position < 0) {
      throw std::runtime_error(
          "AEDAT files without datatables are currently not supported");
    }

    // rapidxml::xml_document<> doc;
    // doc.parse<0>((char *)(ioheader->info_node()->str().c_str()));
    // extract necessary data from XML
    // auto node = doc.first_node();
    // for (rapidxml::xml_node<> *outinfo = node->first_node(); outinfo;
    //      outinfo = outinfo->next_sibling()) {

    //   auto attributes = collect_attributes(outinfo);
    //   if (attributes["name"] != "outInfo") {
    //     continue;
    //   }

    //   for (rapidxml::xml_node<> *child = outinfo->first_node(); child;
    //        child = child->next_sibling()) {
    //     OutInfo info;
    //     auto attributes = collect_attributes(child);
    //     if (!attributes.contains("name")) {
    //       continue;
    //     }

    //     info.name = std::stoi(attributes["name"]);

    //     for (rapidxml::xml_node<> *attr = child->first_node(); attr;
    //          attr = attr->next_sibling()) {
    //       auto attributes = collect_attributes(attr);
    //       if (attributes["key"] == "compression") {
    //         info.compression = attr->value();
    //       } else if (attributes["key"] == "typeIdentifier") {
    //         info.type = OutInfo::to_type(attr->value());
    //       } else if (attributes["name"] == "info") {
    //         for (rapidxml::xml_node<> *info_node = attr->first_node();
    //              info_node; info_node = info_node->next_sibling()) {
    //           auto infos = collect_attributes(info_node);

    //           if (infos["key"] == "sizeX") {
    //             info.size_x = std::stoi(info_node->value());
    //           } else if (infos["key"] == "sizeY") {
    //             info.size_y = std::stoi(info_node->value());
    //           }
    //         }
    //       }
    //     }
    //     outinfos.push_back(info);
    //   }
    // }

    // TODO: Re-introduce XML parsing

    // we have to treat each packet according the compression method used,
    // which can be found in ioheader->compression()
    // assume LZ4 compression for now
    LZ4F_errorCode_t lz4_error =
        LZ4F_createDecompressionContext(&ctx, LZ4F_VERSION);

    if (LZ4F_isError(lz4_error)) {
      const auto message = "Error creating LZ4 decompression context: " +
                           std::string(LZ4F_getErrorName(lz4_error));
      throw std::runtime_error(message);
    }

    // Load data table
    if (fseek(fp.get(), data_table_position, SEEK_SET) != 0) {
      throw std::runtime_error("Failed to locate AEDAT4 data table");
    }
    size_t data_table_size = stat_info.st_size - data_table_position;
    // Ensure buffer is large enough
    if (data_buffer.size() < data_table_size) {
      data_buffer.resize(data_table_size);
      dst_buffer.resize(data_table_size * 2); // Ensure sufficient capacity
    }
    if (fread(data_buffer.data(), sizeof(char), data_table_size, fp.get()) != data_table_size) {
      throw std::runtime_error("Failed to read AEDAT4 data table");
    }
    size_t dst_capacity = dst_buffer.capacity();
    int ret = LZ4F_decompress(ctx, dst_buffer.data(), &dst_capacity, data_buffer.data(), &data_table_size, nullptr);
    if (LZ4F_isError(ret)) {
      std::string message = "Error decompressing AEDAT4 DataTable:" +
                            std::string(LZ4F_getErrorName(ret));
      throw std::runtime_error(message);
    }
    auto root = GetSizePrefixedFileDataTable(dst_buffer.data());

    // Record offsets of event packets
    // TODO: add IMU + frames
    auto packets = root->Table();
    for (size_t i = 0; i < packets->size(); ++i) {
      auto definition = packets->Get(i);
      if (definition->NumElements() == 0) {
        continue;
      }
      const auto stream_id = definition->PacketInfo()->StreamID();
      const size_t offset = definition->ByteOffset();
      switch (stream_id) {
      case 0: {
        event_packet_offsets.push_back(offset - 8); // Include 8 byte header
        total_number_of_events += definition->NumElements();
      }
      // case OutInfo::Type::FRME: {
      //   auto frame_packet = GetSizePrefixedFrame(&dst_buffer[0]);
      //   break;
      // }
      // case OutInfo::Type::IMUS: {
      //   auto imu_packet = GetSizePrefixedImuPacket(&dst_buffer[0]);
      //   break;
      // }
      // case OutInfo::Type::TRIG: {
      //   auto trigger_packet = GetSizePrefixedTriggerPacket(&dst_buffer[0]);
      //   break;
      // }
      default: {
        break;
      }
      }
    }
  }

  bool get_packet() {
    // Test for end of current packet
    if (event_vector) {
      if (packet_events_read >= event_vector->size()) {
        packet_index++; // Move to next packet
        event_vector = nullptr;
      } else {
        return true; // Continue reading current packet
      }
    }

    // Test for end of packets
    if (packet_index >= event_packet_offsets.size()) {
      return false;
    }

    // Load new packet
    packet_events_read = 0;
    // Read packet header
    fseek(fp.get(), event_packet_offsets[packet_index], SEEK_SET);
    int32_t id, size;
    auto id_ret = fread(&id, 4, 1, fp.get());
    auto size_ret = fread(&size, 4, 1, fp.get());
    if (id_ret == 0 || size_ret == 0) { // Error or EOF
      return false;
    }
    if (size == 0) { // Continue if packet is empty
      packet_index++;
      event_vector = nullptr;
      return get_packet();
    }
    if (packet_buffer.size() < size) { // Resize buffer if needed
      packet_buffer.resize(size);
      dst_buffer.resize(size * 4); // x4 ensures buffer is large enough
    }

    // Read packet bytes
    auto packet_ret = fread(&packet_buffer[0], 1, size, fp.get());
    if (packet_ret == 0) { // Error or EOF
      return false;
    }

    // Decompress packet
    size_t decomp_size = size;
    size_t dst_capacity = dst_buffer.capacity();
    auto ret = LZ4F_decompress(ctx, &dst_buffer[0], &dst_capacity,
                               &packet_buffer[0], &decomp_size, nullptr);
    if (LZ4F_isError(ret)) {
      printf("Decompression package error: %s\n", LZ4F_getErrorName(ret));
      return false;
    }
    const auto events = GetSizePrefixedEventPacket(&dst_buffer[0]);
    event_vector = events->elements();
    return true;
  }
};
