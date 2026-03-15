#include <stdio.h>
#include <stdint.h>
#include <libdragon.h>

// Guardrail for the Makefile macro
#ifndef APP_TITLE
    #define APP_TITLE "N64_VI_TEST"
#endif

int main(void) {
    // 1. Initialize bare-metal subsystems
    // We use vi_init instead of display_init to avoid background interrupts 
    // from overriding our custom registers.
    debug_init_usblog();
    vi_init(); 
    timer_init();

    // Disable interrupts to ensure our TICKS_READ loop is the only thing running.
    disable_interrupts();

    // 2. Configure the Hardware Experiment
    // We clone the NTSC preset and modify the H_TOTAL and LEAP values.
    vi_timing_preset_t test = VI_TIMING_NTSC;

    // Pattern 0b01100 (Decimal 12) per Rasky's baseline.
    // Base Terminal Count 3093 (3094 cycles).
    test.vi_h_total = VI_H_TOTAL_SET(0b01100, 3093);

    // LEAP_B (3093) and LEAP_A (4000).
    test.vi_h_total_leap = VI_H_TOTAL_LEAP_SET(3093, 4000); 
    
    // Apply to silicon.
    vi_set_timing_preset(&test);

    // 3. Measurement Loop
    uint32_t timings[16] = {0};
    uint32_t last_time;

    // Wait for the next field to ensure the new timing registers are latched.
    vi_wait_vblank();
    last_time = TICKS_READ();

    for (int i = 0; i < 16; i++) {
        // Poll the line counter (bits 9:1 of VI_V_CURRENT).
        uint32_t current_line = *VI_V_CURRENT >> 1;
        
        while ((*VI_V_CURRENT >> 1) == current_line) {
            /* busy wait for hardware to increment to next line */
        }
        
        uint32_t now = TICKS_READ();
        timings[i] = now - last_time;
        last_time = now;
    }

    // Re-enable interrupts now that the critical timing section is done.
    enable_interrupts();

    // 4. Reporting to USB (SC64 / ISViewer)
    debugf("\n--- %s Hardware Results ---\n", APP_TITLE);
    for (int i = 0; i < 16; i++) {
        // CPU Count runs at 46.875MHz, VI Clock at 62.5MHz.
        // Ratio is 4/3 to convert CPU ticks to VI clock cycles.
        uint32_t vi_ticks = (timings[i] * 4) / 3;
        
        debugf("Line %02d: %lu CPU Ticks (~%lu VI Cycles)\n", i, timings[i], vi_ticks);
    }

    // 5. Shutdown/Hold
    // The screen will likely be black or out of sync on a TV, 
    // but the USB logs in your terminal are the source of truth.
    while (1) {
        /* Keep console powered for debug extraction */
    }

    return 0;
}
