#pragma once

// ═════════════════════════════════════════════════════════════════════════════
//  PacketBridge.h
//
//  Owns the full pipeline from API data to a DDS-ready DataPackageStruct.
//
//  Public interface:
//    init()    — initializes the two API contexts. Call once at startup.
//    process() — runs the full pipeline. Call whenever you need to publish.
//                Accepts a DataPackageStruct by reference and fills it.
//
//  The caller owns the DataPackageStruct and passes it in — this class
//  never creates or manages one itself.
// ═════════════════════════════════════════════════════════════════════════════

#include "../api/api_packet.h"
#include "../include/wire_packet.h"
#include "../dds/DataPackageStruct.h"

// ─────────────────────────────────────────────
//  Simulated API context types
//  In real life these come from the API headers
// ─────────────────────────────────────────────
struct ApiContextA {
    uint32_t handle;
    bool     initialized;
};

struct ApiContextB {
    uint32_t handle;
    bool     initialized;
};


class PacketBridge
{
public:

    PacketBridge();
    ~PacketBridge();  // releases/destroys the API contexts on cleanup

    // ── Public interface ──────────────────────────────────────────────────────

    // Initialize the two API contexts.
    // Must be called before process(). Returns true on success.
    bool init();

    // Run the full pipeline:
    //   API fill → build preamble → transfer to wire → pack into DDS struct
    // The caller passes in a DataPackageStruct which this method fills out.
    // Returns true on success.
    bool process(DataBus::DataPackageStruct& dds_pkg);

private:

    // ── API Contexts ──────────────────────────────────────────────────────────
    // Owned by this class, initialized once in init(), reused in process()
    ApiContextA  m_ctx_a;
    ApiContextB  m_ctx_b;
    bool         m_initialized;

    // ── Working buffers ───────────────────────────────────────────────────────
    // Reused across process() calls to avoid repeated stack allocation
    Header   m_api_header;
    Payload  m_api_payloads[MAX_PAYLOAD_COUNT];

    WirePreamble  m_wire_pre;
    WireHeader    m_wire_header;
    WirePayload   m_wire_payloads[MAX_PAYLOAD_COUNT];

    // ── Private pipeline steps ────────────────────────────────────────────────
    void apiConfigure(ApiContextA& ctx_a, ApiContextB& ctx_b);
    void apiRelease(ApiContextA& ctx_a, ApiContextB& ctx_b);
    void apiFill(ApiContextA& ctx_a, ApiContextB& ctx_b,
                 Header& header, Payload* payloads, uint16_t count);

    Preamble    buildPreamble(const Header& header, uint16_t payload_count);
    void        transferToWire(const Preamble& api_pre,
                               const Header&   api_header,
                               const Payload*  api_payloads);
    bool        packIntoDds(DataBus::DataPackageStruct& dds_pkg);
};