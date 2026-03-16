#include <stdio.h>
#include <stdint.h>
#include <libdragon.h>

// Direct Hardware Addresses 
#define VI_BASE          0xA4400000
#define VI_V_CURRENT     (volatile uint32_t*)(VI_BASE + 0x10)
#define VI_H_SYNC        (volatile uint32_t*)(VI_BASE + 0x1C)
#define VI_H_SYNC_LEAP   (volatile uint32_t*)(VI_BASE + 0x20)

int main(void) {
    debug_init_usblog();
    timer_init();
    
    // 1. Start the VI clock by initializing a standard display
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);

    debugf("Starting Raw VI Leap Experiment...\n");

    // 2. Hardware Override
    // These values are written to "shadow" registers. 
    // They won't take effect until the next V-Blank.
    uint32_t leap_reg_val = (3093 << 16) | 3193;
    uint32_t sync_reg_val = 3093; // Normal H-Sync period

    *VI_H_SYNC_LEAP = leap_reg_val;
    *VI_H_SYNC      = sync_reg_val;

    // 3. The Latch Phase
    // Wait for the current frame to end so the VI latches our new values.
    while ((*VI_V_CURRENT >> 1) != 0); 
    while ((*VI_V_CURRENT >> 1) == 0);

    // 4. The Measurement Phase
    disable_interrupts(); 
    
    uint32_t timings[16];

    // SYNC: Wait for the start of a fresh scanline
    uint32_t start_line = (*VI_V_CURRENT) >> 1;
    while (((*VI_V_CURRENT) >> 1) == start_line);

    uint32_t last_time = TICKS_READ();

    for (int i = 0; i < 16; i++) {
        uint32_t current_line = (*VI_V_CURRENT) >> 1;
        
        // Wait for line transition
        while (((*VI_V_CURRENT) >> 1) == current_line);
        
        uint32_t now = TICKS_READ();
        timings[i] = now - last_time;
        last_time = now;
    }

    enable_interrupts();

    // 5. Reporting results
    debugf("\n--- Measurement Results ---\n");
    for (int i = 0; i < 16; i++) {
        // CPU Ticks (typically 46.875MHz or 93.75MHz depending on console/emu)
        debugf("Line %02d: %lu CPU Ticks\n", i, timings[i]);
    }

    while (1) { /* spin */ }
    return 0;
}