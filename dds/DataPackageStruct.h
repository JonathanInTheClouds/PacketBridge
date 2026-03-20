#pragma once

// ─────────────────────────────────────────────────────────────────────────────
//  DataPackageStruct.h
//
//  MOCK of what rtiddsgen generates from DataPackage.idl.
//  In your real project this file is AUTO-GENERATED — don't hand-edit it.
//  This fake version gives us something to compile and work against.
//
//  RTI Connext traditional C++ API:
//    sequence<octet>  -->  DDS_OctetSeq
// ─────────────────────────────────────────────────────────────────────────────

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstdio>

// ─────────────────────────────────────────────────────────────────────────────
//  DDS_OctetSeq  (mock of RTI's type)
//
//  RTI's real DDS_OctetSeq is a macro-generated class that manages a heap
//  buffer with a maximum and current length.  This mock mirrors that interface
//  closely enough to write real bridging code against.
// ─────────────────────────────────────────────────────────────────────────────
class DDS_OctetSeq {
public:
    DDS_OctetSeq()
        : _buffer(nullptr), _length(0), _maximum(0) {}

    ~DDS_OctetSeq() {
        if (_buffer) {
            free(_buffer);
        }
    }

    // Allocate / resize the internal buffer (mirrors ensure_length)
    bool ensure_length(uint32_t length, uint32_t maximum) {
        if (maximum < length) maximum = length;
        if (maximum > _maximum) {
            uint8_t* nb = static_cast<uint8_t*>(realloc(_buffer, maximum));
            if (!nb) return false;
            _buffer  = nb;
            _maximum = maximum;
        }
        _length = length;
        return true;
    }

    // Direct pointer to the buffer (use this to memcpy your bytes in)
    uint8_t*       get_contiguous_buffer()       { return _buffer; }
    const uint8_t* get_contiguous_buffer() const { return _buffer; }

    uint32_t length()  const { return _length;  }
    uint32_t maximum() const { return _maximum; }

private:
    uint8_t* _buffer;
    uint32_t _length;
    uint32_t _maximum;
};


// ─────────────────────────────────────────────────────────────────────────────
//  DataPackageStruct  (mock of rtiddsgen output for DataPackage.idl)
// ─────────────────────────────────────────────────────────────────────────────
namespace DataBus {

    struct DataPackageStruct {
        uint32_t      source_id;
        uint32_t      dest_id;
        uint32_t      sequence_num;
        uint8_t       message_type;

        // Descriptor fields — mirrors the Preamble so receivers know
        // the layout of the octet blob without having to decode it first
        uint16_t      header_size;          // Size of the API Header struct in bytes
        uint16_t      payload_record_size;  // Size of ONE Payload struct in bytes
        uint16_t      payload_count;        // N — how many payload records are in the blob
        uint32_t      message_length;       // Total byte length of the full serialized packet

        DDS_OctetSeq  payload;              // The raw serialized packet bytes
    };

}  // namespace DataBus
