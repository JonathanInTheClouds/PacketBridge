#include "../include/PacketBridge.h"
#include <cstdio>

int main()
{
    // --- Create and initialize the bridge ---
    PktBridge::PacketBridge bridge;
    if (!bridge.init()) {
        return 1;
    }

    // --- DataPackageStruct lives outside the bridge ---
    //     The larger system owns it and passes it in
    DataBus::DataPackageStruct dds_pkg;

    // --- Run the pipeline ---
    if (!bridge.process(dds_pkg)) {
        return 1;
    }

    // --- At this point dds_pkg is fully populated ---
    printf("\n=== Ready to publish ===\n");
    printf("  source_id      : 0x%04X\n",  dds_pkg.source_id);
    printf("  dest_id        : 0x%04X\n",  dds_pkg.dest_id);
    printf("  payload_count  : %u\n",       dds_pkg.payload_count);
    printf("  message_length : %u bytes\n", dds_pkg.message_length);
    printf("  octet buffer   : %u bytes\n", dds_pkg.payload.length());

    // In real life:
    // DataPackageStructDataWriter->write(dds_pkg, DDS_HANDLE_NIL);

    return 0;
}