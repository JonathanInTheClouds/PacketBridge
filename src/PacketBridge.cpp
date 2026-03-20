#include "../include/PacketBridge.h"
#include <cstring>
#include <cstdio>

namespace PktBridge {




// ─────────────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────────────
PacketBridge::PacketBridge()
    : m_ctx_a{0, false}
    , m_ctx_b{0, false}
    , m_initialized(false)
    , m_api_header{}
    , m_api_payloads{}
    , m_wire_pre{}
    , m_wire_header{}
    , m_wire_payloads{}
{
}


// ─────────────────────────────────────────────────────────────────────────────
//  Destructor
//
//  Automatically called when the PacketBridge goes out of scope or is deleted.
//  Passes both contexts to the API release/destroy function to clean up.
//  In real life: api_release(&m_ctx_a, &m_ctx_b) or similar.
// ─────────────────────────────────────────────────────────────────────────────
PacketBridge::~PacketBridge()
{
    if (m_initialized) {
        apiRelease(m_ctx_a, m_ctx_b);
    }
}



//
//  Creates and configures the two API contexts.
//  In real life this is where you call the API's init/config functions,
//  passing in the contexts by reference or pointer.
//  Must be called once before process().
// ─────────────────────────────────────────────────────────────────────────────
bool PacketBridge::init()
{
    // Step 1 — initialize both contexts
    m_ctx_a = {0x0001, false};
    m_ctx_b = {0x0002, false};

    // Step 2 — pass them to the API configuration function
    //          In real life: api_configure(m_ctx_a, m_ctx_b);
    apiConfigure(m_ctx_a, m_ctx_b);

    if (!m_ctx_a.initialized || !m_ctx_b.initialized) {
        printf("ERROR: API context initialization failed\n");
        return false;
    }

    m_initialized = true;
    printf("PacketBridge initialized successfully\n");
    return true;
}


// ─────────────────────────────────────────────────────────────────────────────
//  process()
//
//  Runs the full pipeline. The caller passes in a DataPackageStruct
//  which this method fills. Returns true on success.
// ─────────────────────────────────────────────────────────────────────────────
bool PacketBridge::process(DataBus::DataPackageStruct& dds_pkg)
{
    if (!m_initialized) {
        printf("ERROR: PacketBridge not initialized — call init() first\n");
        return false;
    }

    const uint16_t PAYLOAD_COUNT = 2048;

    // ── Step 1: API fills the Header and Payload array ────────────────────────
    //           Pass our member buffers in — API populates them
    apiFill(m_ctx_a, m_ctx_b, m_api_header, m_api_payloads, PAYLOAD_COUNT);

    // ── Step 2: Build the Preamble from what the API gave us ─────────────────
    Preamble api_preamble = buildPreamble(m_api_header, PAYLOAD_COUNT);

    // ── Step 3: Transfer everything into pragma-packed Wire structs ───────────
    transferToWire(api_preamble, m_api_header, m_api_payloads);

    // ── Step 4: Pack Wire structs into the DDS DataPackageStruct ─────────────
    if (!packIntoDds(dds_pkg)) {
        return false;
    }

    return true;
}


// ═════════════════════════════════════════════════════════════════════════════
//  PRIVATE METHODS
// ═════════════════════════════════════════════════════════════════════════════

// ─────────────────────────────────────────────────────────────────────────────
//  apiConfigure()
//  Simulates passing the two contexts to the API's configuration function.
//  In real life: api_configure(&ctx_a, &ctx_b) or similar.
// ─────────────────────────────────────────────────────────────────────────────
void PacketBridge::apiConfigure(ApiContextA& ctx_a, ApiContextB& ctx_b)
{
    // Simulate the API marking the contexts as ready
    ctx_a.initialized = true;
    ctx_b.initialized = true;
}


// ─────────────────────────────────────────────────────────────────────────────
//  apiRelease()
//  Simulates passing the two contexts to the API's release/destroy function.
//  In real life: api_release(&ctx_a, &ctx_b) or similar.
// ─────────────────────────────────────────────────────────────────────────────
void PacketBridge::apiRelease(ApiContextA& ctx_a, ApiContextB& ctx_b)
{
    // Simulate the API cleaning up the contexts
    ctx_a.initialized = false;
    ctx_a.handle      = 0;
    ctx_b.initialized = false;
    ctx_b.handle      = 0;

    printf("PacketBridge contexts released\n");
}


// ─────────────────────────────────────────────────────────────────────────────
//  apiFill()
//  Simulates the API filling the Header and Payload array using the contexts.
//  In real life: api_get_data(&ctx_a, &ctx_b, &header, payloads, count)
// ─────────────────────────────────────────────────────────────────────────────
void PacketBridge::apiFill(ApiContextA& ctx_a, ApiContextB& ctx_b,
                            Header&  header,
                            Payload* payloads,
                            uint16_t count)
{
    (void)ctx_a;  // used by real API — suppress unused warning for now
    (void)ctx_b;

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
    for (uint16_t i = 0; i < count; ++i) {
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
//  buildPreamble()
// ─────────────────────────────────────────────────────────────────────────────
Preamble PacketBridge::buildPreamble(const Header& header, uint16_t payload_count)
{
    (void)header;

    Preamble pre;
    pre.magic          = 0xDEAD;
    pre.version        = 1;
    pre.header_size    = static_cast<uint16_t>(sizeof(WireHeader));
    pre.payload_size   = static_cast<uint16_t>(sizeof(WirePayload));
    pre.payload_count  = payload_count;
    pre.message_length = static_cast<uint32_t>(
                             sizeof(WirePreamble)                   +
                             sizeof(WireHeader)                     +
                             payload_count * sizeof(WirePayload));
    return pre;
}


// ─────────────────────────────────────────────────────────────────────────────
//  transferToWire()
//  Copies API structs field by field into our pragma-packed Wire members.
// ─────────────────────────────────────────────────────────────────────────────
void PacketBridge::transferToWire(const Preamble& api_pre,
                                   const Header&   api_header,
                                   const Payload*  api_payloads)
{
    // --- Preamble ---
    m_wire_pre.magic          = api_pre.magic;
    m_wire_pre.version        = api_pre.version;
    m_wire_pre.header_size    = api_pre.header_size;
    m_wire_pre.payload_size   = api_pre.payload_size;
    m_wire_pre.payload_count  = api_pre.payload_count;
    m_wire_pre.message_length = api_pre.message_length;

    // --- Header.routing ---
    m_wire_header.routing.source_id    = api_header.routing.source_id;
    m_wire_header.routing.dest_id      = api_header.routing.dest_id;
    m_wire_header.routing.sequence_num = api_header.routing.sequence_num;
    m_wire_header.routing.message_type = api_header.routing.message_type;
    m_wire_header.routing.reserved     = api_header.routing.reserved;

    // --- Header.session ---
    m_wire_header.session.session_id       = api_header.session.session_id;
    m_wire_header.session.timestamp_offset = api_header.session.timestamp_offset;
    m_wire_header.session.checksum         = api_header.session.checksum;

    // --- Header.control ---
    m_wire_header.control.flags    = api_header.control.flags;
    m_wire_header.control.priority = api_header.control.priority;
    m_wire_header.control.reserved = api_header.control.reserved;

    // --- Payloads ---
    for (uint16_t i = 0; i < api_pre.payload_count; ++i) {
        m_wire_payloads[i].tag            = api_payloads[i].tag;
        m_wire_payloads[i].status         = api_payloads[i].status;
        m_wire_payloads[i].reserved       = api_payloads[i].reserved;
        m_wire_payloads[i].value_signed   = api_payloads[i].value_signed;
        m_wire_payloads[i].value_unsigned = api_payloads[i].value_unsigned;
        m_wire_payloads[i].value_float    = api_payloads[i].value_float;
        m_wire_payloads[i].timestamp      = api_payloads[i].timestamp;
    }
}


// ─────────────────────────────────────────────────────────────────────────────
//  packIntoDds()
//  memcpy's the Wire member structs into the DDS_OctetSeq buffer.
// ─────────────────────────────────────────────────────────────────────────────
bool PacketBridge::packIntoDds(DataBus::DataPackageStruct& dds_pkg)
{
    // --- Descriptor fields ---
    dds_pkg.source_id           = m_wire_header.routing.source_id;
    dds_pkg.dest_id             = m_wire_header.routing.dest_id;
    dds_pkg.sequence_num        = m_wire_header.routing.sequence_num;
    dds_pkg.message_type        = m_wire_header.routing.message_type;
    dds_pkg.header_size         = m_wire_pre.header_size;
    dds_pkg.payload_record_size = m_wire_pre.payload_size;
    dds_pkg.payload_count       = m_wire_pre.payload_count;
    dds_pkg.message_length      = m_wire_pre.message_length;

    // --- Allocate octet buffer ---
    const uint32_t total_bytes = m_wire_pre.message_length;
    if (!dds_pkg.payload.ensure_length(total_bytes, total_bytes)) {
        printf("ERROR: failed to allocate octet buffer (%u bytes)\n", total_bytes);
        return false;
    }

    uint8_t*  buf    = dds_pkg.payload.get_contiguous_buffer();
    uint32_t  offset = 0;

    // --- memcpy Wire structs sequentially into the buffer ---
    memcpy(buf + offset, &m_wire_pre, sizeof(WirePreamble));
    offset += sizeof(WirePreamble);

    memcpy(buf + offset, &m_wire_header, sizeof(WireHeader));
    offset += sizeof(WireHeader);

    memcpy(buf + offset, m_wire_payloads,
           m_wire_pre.payload_count * sizeof(WirePayload));
    offset += m_wire_pre.payload_count * sizeof(WirePayload);

    // --- Sanity check ---
    if (offset != total_bytes) {
        printf("ERROR: offset mismatch — wrote %u bytes, expected %u\n",
               offset, total_bytes);
        return false;
    }

    return true;
}

}  // namespace PktBridge