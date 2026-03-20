#include "../api/api_packet.h"
#include "../include/wire_packet.h"
#include "../dds/DataPackageStruct.h"
#include <cstring>
#include <cstdio>

// ═════════════════════════════════════════════════════════════════════════════
//
//  bridge.cpp — full flow in one place
//
//  Step 1 | simulate_api_fill()
//           Simulates what the API does for us: fills a Header and a
//           Payload array with data. In real life the API does this —
//           we just call it and get back populated structs.
//
//  Step 2 | build_preamble()
//           We have no preamble yet — we derive it entirely from the
//           Header and payload array the API gave us.
//
//  Step 3 | transfer_to_wire()
//           Copy every field from the API structs into our pragma-packed
//           Wire structs. Field by field, struct by struct.
//           Once in Wire structs the data is safe to memcpy into the
//           DDS octet sequence without any hidden padding gaps.
//
// ═════════════════════════════════════════════════════════════════════════════


// ─────────────────────────────────────────────────────────────────────────────
//  STEP 1 — Simulate the API filling the Header and Payload array
//
//  In real life you call the API and it populates these for you.
//  This function stands in for that — it represents the boundary where
//  API data enters our system.
//
//  Parameters:
//    header        — API Header struct to be filled
//    payloads      — API Payload array to be filled
//    payload_count — how many payload slots to populate (must be <= MAX_PAYLOAD_COUNT)
// ─────────────────────────────────────────────────────────────────────────────
void simulate_api_fill(Header&  header,
                       Payload* payloads,
                       uint16_t payload_count)
{
    // --- Header.routing ---
    header.routing.source_id    = 0x0001;
    header.routing.dest_id      = 0x0002;
    header.routing.sequence_num = 7;
    header.routing.message_type = 0x01;
    header.routing.reserved     = 0;

    // --- Header.session ---
    header.session.session_id       = 0xCAFEBABE;
    header.session.timestamp_offset = 0;
    header.session.checksum         = 0;

    // --- Header.control ---
    header.control.flags    = 0x00;
    header.control.priority = 5;
    header.control.reserved = 0;

    // --- Payloads ---
    for (uint16_t i = 0; i < payload_count; ++i) {
        payloads[i].tag            = static_cast<uint16_t>(0x0A00 + i);
        payloads[i].status         = 0;
        payloads[i].reserved       = 0;
        payloads[i].value_signed   = -100 + (i * 10);
        payloads[i].value_unsigned = i * 42u;
        payloads[i].value_float    = 3.14f * i;
        payloads[i].timestamp      = 1700000000u + i;
    }
}


// ─────────────────────────────────────────────────────────────────────────────
//  STEP 2 — Build the Preamble from what the API gave us
//
//  The API gives us the Header and payloads but not the Preamble.
//  We derive it entirely from the data we already have.
//  Note: sizes here are of the WIRE structs, not the API structs,
//  since that's what will actually be on the wire.
// ─────────────────────────────────────────────────────────────────────────────
Preamble build_preamble(const Header& header, uint16_t payload_count)
{
    // header is passed in to make the dependency explicit even though
    // we don't read from it yet — it may carry version/type info later
    (void)header;

    Preamble pre;
    pre.magic          = 0xDEAD;
    pre.version        = 1;
    pre.header_size    = static_cast<uint16_t>(sizeof(WireHeader));   // wire size
    pre.payload_size   = static_cast<uint16_t>(sizeof(WirePayload));  // wire size
    pre.payload_count  = payload_count;
    pre.message_length = static_cast<uint32_t>(
                             sizeof(WirePreamble)                    +
                             sizeof(WireHeader)                      +
                             payload_count * sizeof(WirePayload));
    return pre;
}


// ─────────────────────────────────────────────────────────────────────────────
//  STEP 3 — Transfer API structs into our pragma-packed Wire structs
//
//  This is the bridge. We read every field from the API structs (which may
//  have padding) and write them into our Wire structs (which are guaranteed
//  to have no padding). Field names and types are identical — the only
//  difference is the pragma on our side.
//
//  Once this is done the Wire structs are safe to memcpy into the
//  DDS octet sequence.
// ─────────────────────────────────────────────────────────────────────────────
void transfer_to_wire(const Preamble&  api_pre,
                      const Header&    api_header,
                      const Payload*   api_payloads,
                      WirePreamble&    wire_pre,
                      WireHeader&      wire_header,
                      WirePayload*     wire_payloads)
{
    // --- Preamble ---
    wire_pre.magic          = api_pre.magic;
    wire_pre.version        = api_pre.version;
    wire_pre.header_size    = api_pre.header_size;
    wire_pre.payload_size   = api_pre.payload_size;
    wire_pre.payload_count  = api_pre.payload_count;
    wire_pre.message_length = api_pre.message_length;

    // --- Header.routing ---
    wire_header.routing.source_id    = api_header.routing.source_id;
    wire_header.routing.dest_id      = api_header.routing.dest_id;
    wire_header.routing.sequence_num = api_header.routing.sequence_num;
    wire_header.routing.message_type = api_header.routing.message_type;
    wire_header.routing.reserved     = api_header.routing.reserved;

    // --- Header.session ---
    wire_header.session.session_id       = api_header.session.session_id;
    wire_header.session.timestamp_offset = api_header.session.timestamp_offset;
    wire_header.session.checksum         = api_header.session.checksum;

    // --- Header.control ---
    wire_header.control.flags    = api_header.control.flags;
    wire_header.control.priority = api_header.control.priority;
    wire_header.control.reserved = api_header.control.reserved;

    // --- Payloads ---
    for (uint16_t i = 0; i < api_pre.payload_count; ++i) {
        wire_payloads[i].tag            = api_payloads[i].tag;
        wire_payloads[i].status         = api_payloads[i].status;
        wire_payloads[i].reserved       = api_payloads[i].reserved;
        wire_payloads[i].value_signed   = api_payloads[i].value_signed;
        wire_payloads[i].value_unsigned = api_payloads[i].value_unsigned;
        wire_payloads[i].value_float    = api_payloads[i].value_float;
        wire_payloads[i].timestamp      = api_payloads[i].timestamp;
    }
}


// ─────────────────────────────────────────────────────────────────────────────
//  main — drives the full flow
// ─────────────────────────────────────────────────────────────────────────────
int main()
{
    const uint16_t PAYLOAD_COUNT = 2048;

    // ── STEP 1: API gives us a filled Header and Payload array ───────────────

    Header  api_header;
    Payload api_payloads[MAX_PAYLOAD_COUNT] = {};  // zero-init unused slots

    simulate_api_fill(api_header, api_payloads, PAYLOAD_COUNT);

    printf("=== STEP 1: API data received ===\n");
    printf("  source_id    : 0x%04X\n", api_header.routing.source_id);
    printf("  session_id   : 0x%08X\n", api_header.session.session_id);
    printf("  payload[0]   : tag=0x%04X  val=%d\n",
           api_payloads[0].tag, api_payloads[0].value_signed);
    printf("  payload[2047]: tag=0x%04X  val=%d\n",
           api_payloads[2047].tag, api_payloads[2047].value_signed);

    // ── STEP 2: Build the Preamble from what we have ─────────────────────────

    Preamble api_preamble = build_preamble(api_header, PAYLOAD_COUNT);

    printf("\n=== STEP 2: Preamble derived ===\n");
    printf("  magic          : 0x%04X\n",  api_preamble.magic);
    printf("  header_size    : %u bytes\n", api_preamble.header_size);
    printf("  payload_size   : %u bytes\n", api_preamble.payload_size);
    printf("  payload_count  : %u\n",       api_preamble.payload_count);
    printf("  message_length : %u bytes\n", api_preamble.message_length);

    // ── STEP 3: Transfer everything into our pragma-packed Wire structs ───────

    WirePreamble  wire_pre;
    WireHeader    wire_header;
    WirePayload   wire_payloads[MAX_PAYLOAD_COUNT] = {};

    transfer_to_wire(api_preamble, api_header, api_payloads,
                     wire_pre, wire_header, wire_payloads);

    printf("\n=== STEP 3: Transferred to Wire structs ===\n");
    printf("  sizeof(Preamble)     : %zu  (API, may have padding)\n", sizeof(Preamble));
    printf("  sizeof(WirePreamble) : %zu  (ours, no padding)\n",      sizeof(WirePreamble));
    printf("  sizeof(Header)       : %zu  (API, may have padding)\n", sizeof(Header));
    printf("  sizeof(WireHeader)   : %zu  (ours, no padding)\n",      sizeof(WireHeader));
    printf("  sizeof(Payload)      : %zu  (API, may have padding)\n", sizeof(Payload));
    printf("  sizeof(WirePayload)  : %zu  (ours, no padding)\n",      sizeof(WirePayload));
    printf("  wire_header.routing.source_id : 0x%04X\n", wire_header.routing.source_id);
    printf("  wire_payloads[0].tag          : 0x%04X\n", wire_payloads[0].tag);

    // ── At this point wire_pre, wire_header, wire_payloads are ready ─────────
    //    Next step: memcpy them into the DDS_OctetSeq buffer

    return 0;
}
