#pragma once

// ═════════════════════════════════════════════════════════════════════════════
//  api_packet.h
//
//  SIMULATED API STRUCTS — these represent what the third-party API gives us.
//
//  Key points:
//    - We do NOT own these definitions
//    - No #pragma pack — the API doesn't guarantee tight packing
//    - The compiler may insert padding between fields for alignment
//    - We never memcpy these structs wholesale into a wire buffer
//    - We only ever read individual fields out of them
// ═════════════════════════════════════════════════════════════════════════════

#include <cstdint>

// Maximum payload slots — dictated by the API spec
static constexpr uint16_t MAX_PAYLOAD_COUNT = 2048;

// ─────────────────────────────────────────────
//  PREAMBLE  (from API)
// ─────────────────────────────────────────────
struct Preamble {
    uint16_t  magic;            // Sync/magic bytes e.g. 0xDEAD
    uint8_t   version;          // Protocol version
    uint16_t  header_size;      // Size of the Header struct in bytes
    uint16_t  payload_size;     // Size of ONE Payload struct in bytes
    uint16_t  payload_count;    // N — how many payloads are populated
    uint32_t  message_length;   // Total message size in bytes
};

// ─────────────────────────────────────────────
//  HEADER nested structs  (from API)
// ─────────────────────────────────────────────

struct RoutingInfo {
    uint16_t  source_id;
    uint16_t  dest_id;
    uint16_t  sequence_num;
    uint8_t   message_type;
    uint8_t   reserved;         // <-- compiler may pad after this
};

struct SessionInfo {
    uint32_t  session_id;
    int32_t   timestamp_offset;
    uint32_t  checksum;
};

struct ControlInfo {
    uint8_t   flags;
    int16_t   priority;         // <-- compiler may pad before this (alignment)
    uint8_t   reserved;
};

struct Header {
    RoutingInfo  routing;
    SessionInfo  session;
    ControlInfo  control;
};

// ─────────────────────────────────────────────
//  PAYLOAD  (from API)
// ─────────────────────────────────────────────
struct Payload {
    uint16_t  tag;
    uint8_t   status;
    uint8_t   reserved;
    int32_t   value_signed;
    uint32_t  value_unsigned;
    float     value_float;
    uint32_t  timestamp;
};

// ─────────────────────────────────────────────
//  PACKET  (from API)
//  The full message as the API presents it to us.
//  payloads[0 .. preamble.payload_count - 1] are valid.
// ─────────────────────────────────────────────
struct Packet {
    Preamble  preamble;
    Header    header;
    Payload   payloads[MAX_PAYLOAD_COUNT];
};
