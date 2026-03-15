#include <stdio.h>
#include <stdint.h>
#include <libdragon.h>
#include <n64sys.h> // For register access
#include <vi.h>     // For vi_set_timing_preset and structs

// If VI_V_CURRENT is still complaining, use the absolute address 
// or the libdragon name:
#define VI_V_CURRENT_REG ((volatile uint32_t*)0xA4400010)

int main(void) {
    debug_init_usblog();
    // In current libdragon, vi_init is internal. 
    // You can usually just start with:
    timer_init();

    disable_interrupts();

    // The struct name is actually vi_timing_t in some versions, 
    // but let's stick to the preset logic. 
    // If vi_timing_preset_t fails, we use the raw struct:
    vi_timing_t test = *vi_get_timing(); // Clone current hardware state

    // Manually adjust the fields in the struct
    // Pattern 12 | Base 3093
    test.h_sync = (12 << 16) | 3093; 
    // LEAP_B (3093) | LEAP_A (4000)
    test.h_sync_leap = (3093 << 16) | 4000;
    
    // Apply
    vi_write_config(&test);

    uint32_t timings[16] = {0};
    uint32_t last_time;

    // Manual VBlank wait using the register if the function is missing
    while (!(*VI_V_CURRENT_REG & 1)); 
    last_time = TICKS_READ();

    for (int i = 0; i < 16; i++) {
        uint32_t current_line = *VI_V_CURRENT_REG >> 1;
        while ((*VI_V_CURRENT_REG >> 1) == current_line);
        
        uint32_t now = TICKS_READ();
        timings[i] = now - last_time;
        last_time = now;
    }

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
