#include <stdio.h>
#include <stdint.h>
#include <libdragon.h>

// If your libdragon version doesn't expose these in libdragon.h, 
// we define the hardware address clearly:
#define VI_V_CURRENT_REG ((volatile uint32_t*)0xA4400010)

int main(void) {
    debug_init_usblog();
    timer_init();
    
    // We must initialize the display to start the VI hardware clock
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);

    // FIX: Using the correct libdragon struct and function names
    // Note: If vi_set_timing_preset is still throwing errors, your 
    // toolchain might be older than the 'vifx' branch.
    vi_timing_preset_t test = VI_TIMING_NTSC;
    
    // Manually setting the register values in the struct
    // These macros are sometimes internal, so we'll use standard shifts if needed
    test.h_total = (0b01100 << 16) | 400; 
    test.h_total_leap = (400 << 16) | 800;
    
    // In many libdragon versions, this is actually:
    vi_write_timing(&test); 

    disable_interrupts();

    // Replaced vi_wait_vblank with a manual check to avoid "implicit declaration"
    while ((*VI_V_CURRENT_REG >> 1) != 0); 
    while ((*VI_V_CURRENT_REG >> 1) == 0);

    uint32_t timings[16] = {0};

    // Sync to line boundary
    uint32_t sync_line = *VI_V_CURRENT_REG >> 1;
    while ((*VI_V_CURRENT_REG >> 1) == sync_line) { /* spin */ }

    uint32_t last_time = TICKS_READ();

    for (int i = 0; i < 16; i++) {
        uint32_t current_line = *VI_V_CURRENT_REG >> 1;
        while ((*VI_V_CURRENT_REG >> 1) == current_line) { }
        
        uint32_t now = TICKS_READ();
        timings[i] = now - last_time;
        last_time = now;
    }

    enable_interrupts();

    debugf("\n--- Results ---\n");
    for (int i = 0; i < 16; i++) {
        debugf("Line %02d: %lu ticks\n", i, timings[i]);
    }

    while (1) {}
    return 0;
}