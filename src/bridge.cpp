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
//  Step 4 | pack_into_dds()
//           memcpy the Wire structs sequentially into the DDS_OctetSeq
//           buffer inside the DataPackageStruct. Also fills the descriptor
//           fields on the struct so subscribers can read metadata without
//           touching the octet blob. At this point the struct is ready
//           to hand to a DDS DataWriter.
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
//  STEP 4 — Pack Wire structs into the DDS DataPackageStruct
//
//  Now that data is in our pragma-packed Wire structs we can safely
//  memcpy them into the DDS_OctetSeq buffer. The layout in the buffer is:
//
//    [ WirePreamble | WireHeader | WirePayload[0] | WirePayload[1] ... ]
//
//  We also fill the descriptor fields directly on the DataPackageStruct
//  so subscribers can read routing/sizing info without decoding the blob.
// ─────────────────────────────────────────────────────────────────────────────
bool pack_into_dds(const WirePreamble&         wire_pre,
                   const WireHeader&            wire_header,
                   const WirePayload*           wire_payloads,
                   DataBus::DataPackageStruct&  dds_pkg)
{
    // --- 1. Fill descriptor fields directly on the DDS struct ---
    //        Subscribers can read these without touching the octet blob
    dds_pkg.source_id           = wire_header.routing.source_id;
    dds_pkg.dest_id             = wire_header.routing.dest_id;
    dds_pkg.sequence_num        = wire_header.routing.sequence_num;
    dds_pkg.message_type        = wire_header.routing.message_type;
    dds_pkg.header_size         = wire_pre.header_size;
    dds_pkg.payload_record_size = wire_pre.payload_size;
    dds_pkg.payload_count       = wire_pre.payload_count;
    dds_pkg.message_length      = wire_pre.message_length;

    // --- 2. Allocate the octet buffer to fit the full wire message ---
    const uint32_t total_bytes = wire_pre.message_length;
    if (!dds_pkg.payload.ensure_length(total_bytes, total_bytes)) {
        printf("ERROR: failed to allocate octet buffer (%u bytes)\n", total_bytes);
        return false;
    }

    // --- 3. Get a raw pointer to the start of the buffer ---
    uint8_t*  buf    = dds_pkg.payload.get_contiguous_buffer();
    uint32_t  offset = 0;

    // --- 4. memcpy each Wire struct in sequentially ---

    // Preamble
    memcpy(buf + offset, &wire_pre, sizeof(WirePreamble));
    offset += sizeof(WirePreamble);

    // Header
    memcpy(buf + offset, &wire_header, sizeof(WireHeader));
    offset += sizeof(WireHeader);

    // Payloads — only the populated count, not MAX_PAYLOAD_COUNT
    memcpy(buf + offset, wire_payloads,
           wire_pre.payload_count * sizeof(WirePayload));
    offset += wire_pre.payload_count * sizeof(WirePayload);

    // --- 5. Sanity check ---
    if (offset != total_bytes) {
        printf("ERROR: offset mismatch — wrote %u bytes, expected %u\n",
               offset, total_bytes);
        return false;
    }

    return true;
}


// ─────────────────────────────────────────────────────────────────────────────
//  main — drives the full flow
// ─────────────────────────────────────────────────────────────────────────────
int main()
{
    const uint16_t PAYLOAD_COUNT = 2048;

    // ── STEP 1: API gives us a filled Header and Payload array ───────────────

    Header  api_header;
    Payload api_payloads[MAX_PAYLOAD_COUNT] = {};

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

    // ── STEP 4: Pack Wire structs into the DDS DataPackageStruct ─────────────

    DataBus::DataPackageStruct dds_pkg;

    if (!pack_into_dds(wire_pre, wire_header, wire_payloads, dds_pkg)) {
        return 1;
    }

    printf("\n=== STEP 4: Packed into DDS DataPackageStruct ===\n");
    printf("  dds_pkg.source_id           : 0x%04X\n",  dds_pkg.source_id);
    printf("  dds_pkg.dest_id             : 0x%04X\n",  dds_pkg.dest_id);
    printf("  dds_pkg.sequence_num        : %u\n",       dds_pkg.sequence_num);
    printf("  dds_pkg.message_type        : 0x%02X\n",  dds_pkg.message_type);
    printf("  dds_pkg.header_size         : %u bytes\n", dds_pkg.header_size);
    printf("  dds_pkg.payload_record_size : %u bytes\n", dds_pkg.payload_record_size);
    printf("  dds_pkg.payload_count       : %u\n",       dds_pkg.payload_count);
    printf("  dds_pkg.message_length      : %u bytes\n", dds_pkg.message_length);
    printf("  octet buffer length         : %u bytes\n", dds_pkg.payload.length());
    printf("\n  Ready to publish via DDS DataWriter.\n");
    printf("  e.g. DataPackageStructDataWriter->write(dds_pkg, DDS_HANDLE_NIL);\n");

    return 0;
}