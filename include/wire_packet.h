#pragma once

// ═════════════════════════════════════════════════════════════════════════════
//  wire_packet.h
//
//  OUR WIRE STRUCTS — identical layout to api_packet.h, just pragma packed.
//
//  Rules:
//    - Every struct here must mirror its API counterpart exactly
//      (same field names, same types, same order, same nesting)
//    - The ONLY difference is the #pragma pack(1) wrapper
//    - If the API structs change, these must be updated to match
// ═════════════════════════════════════════════════════════════════════════════

#include <cstdint>

namespace PktBridge {

#pragma pack(push, 1)

// ─────────────────────────────────────────────
//  WIRE PREAMBLE  — mirrors struct Preamble
// ─────────────────────────────────────────────
struct WirePreamble {
    uint16_t  magic;
    uint8_t   version;
    uint16_t  header_size;
    uint16_t  payload_size;
    uint16_t  payload_count;
    uint32_t  message_length;
};

// ─────────────────────────────────────────────
//  WIRE HEADER nested structs
//  mirrors RoutingInfo, SessionInfo, ControlInfo
// ─────────────────────────────────────────────
struct WireRoutingInfo {
    uint16_t  source_id;
    uint16_t  dest_id;
    uint16_t  sequence_num;
    uint8_t   message_type;
    uint8_t   reserved;
};

struct WireSessionInfo {
    uint32_t  session_id;
    int32_t   timestamp_offset;
    uint32_t  checksum;
};

struct WireControlInfo {
    uint8_t   flags;
    int16_t   priority;
    uint8_t   reserved;
};

// ─────────────────────────────────────────────
//  WIRE HEADER — mirrors struct Header
// ─────────────────────────────────────────────
struct WireHeader {
    WireRoutingInfo  routing;
    WireSessionInfo  session;
    WireControlInfo  control;
};

// ─────────────────────────────────────────────
//  WIRE PAYLOAD — mirrors struct Payload
// ─────────────────────────────────────────────
struct WirePayload {
    uint16_t  tag;
    uint8_t   status;
    uint8_t   reserved;
    int32_t   value_signed;
    uint32_t  value_unsigned;
    float     value_float;
    uint32_t  timestamp;
};

#pragma pack(pop)

}  // namespace PktBridge