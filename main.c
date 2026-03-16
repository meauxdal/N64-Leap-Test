#include <stdio.h>
#include <stdint.h>
#include <libdragon.h>

int main(void) {
    // 1. Initialize Subsystems
    debug_init_usblog();
    debug_init_isviewer(); // Support for both USB and Emulators
    timer_init();
    
    // We need a display initialized so the VI is actually running
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);

    // 2. Define and Apply Hacked Timing
    // Cloning NTSC as a baseline
    vi_timing_preset_t test = VI_TIMING_NTSC;
    
    /* H_TOTAL: Sets the normal line duration.
       H_TOTAL_LEAP: Sets the alternate line durations.
       The leap values will alternate based on the leap mask.
    */
    test.vi_h_total = VI_H_TOTAL_SET(0b01100, 400); 
    test.vi_h_total_leap = VI_H_TOTAL_LEAP_SET(400, 800);
    
    vi_set_timing_preset(&test);

    // 3. The Measurement Phase
    // Disable interrupts so the CPU doesn't jump away to handle a timer/button
    disable_interrupts();

    // MUST wait for a V-Blank so the shadow registers latch the new preset 
    vi_wait_vblank();

    uint32_t timings[16] = {0};

    // Sync to a fresh line boundary so timings[0] isn't a partial line 
    uint32_t sync_line = *VI_V_CURRENT >> 1;
    while ((*VI_V_CURRENT >> 1) == sync_line) { /* spin */ }

    uint32_t last_time = TICKS_READ();

    for (int i = 0; i < 16; i++) {
        uint32_t current_line = *VI_V_CURRENT >> 1;
        
        // Wait for the hardware to increment the line counter
        while ((*VI_V_CURRENT >> 1) == current_line) { /* spin */ }
        
        uint32_t now = TICKS_READ();
        timings[i] = now - last_time;
        last_time = now;
    }

    enable_interrupts();

    // 4. Reporting
    debugf("\n--- VI Leap Measurement Results ---\n");
    for (int i = 0; i < 16; i++) {
        // Converting ticks to microseconds for easier reading
        debugf("Line %02d: %lu ticks (%lu us)\n", i, timings[i], TICKS_TO_US(timings[i]));
    }

    debugf("Experiment Complete.\n");

    while (1) {
        // Keep the CPU alive to maintain the debug connection
    }

    return 0;
}
