#include <stdio.h>
#include <stdint.h>
#include <libdragon.h>

#define VI_BASE          0xA4400000
#define VI_V_CURRENT     (volatile uint32_t*)(VI_BASE + 0x10)
#define VI_BURST         (volatile uint32_t*)(VI_BASE + 0x14)
#define VI_H_TOTAL       (volatile uint32_t*)(VI_BASE + 0x1C)
#define VI_H_TOTAL_LEAP  (volatile uint32_t*)(VI_BASE + 0x20)
#define VI_H_VIDEO       (volatile uint32_t*)(VI_BASE + 0x24)

int main(void) {
    debug_init_usblog();
    timer_init();
    // Standard init to get the clocks and USB stable
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);

    // 1. Target: 3200 (documented "Over" baseline)
    // This should result in ~3090 CPU ticks per line
    uint32_t h_target = 3200; 

    debugf("--- Sustained H_TOTAL Test ---\n");

    while(1) {
        // Hammer the base and the leap to ensure total alignment
        *VI_H_TOTAL      = (0x00 << 16) | (h_target - 1); // Documented: "One less than total"
        *VI_H_TOTAL_LEAP = (h_target << 16) | h_target;
        
        // Wait for field start (Line 0)
        while ((*VI_V_CURRENT >> 1) != 0);
        while ((*VI_V_CURRENT >> 1) == 0);

        disable_interrupts();
        uint32_t timings[64];
        uint32_t snap_h_total;
        uint32_t snap_h_leap;
        
        uint32_t last_time = TICKS_READ();

        for (int i = 0; i < 64; i++) {
            uint32_t line = *VI_V_CURRENT >> 1;
            while ((*VI_V_CURRENT >> 1) == line);
            uint32_t now = TICKS_READ();
            timings[i] = now - last_time;
            last_time = now;
        }
        
        // Capture register state immediately after measurement
        snap_h_total = *VI_H_TOTAL;
        snap_h_leap  = *VI_H_TOTAL_LEAP;
        enable_interrupts();

        // 2. Visible Debug Output
        debugf("REGS AT MEASURE: H_TOTAL:%08lX  LEAP:%08lX\n", snap_h_total, snap_h_leap);
        for (int i = 0; i < 64; i++) {
            debugf("%lu ", timings[i]);
            if((i + 1) % 8 == 0) debugf("\n");
        }
        debugf("----------------------------------------------\n");

        // Throttle to keep USB buffer happy
        for(volatile int d=0; d<5000000; d++);
    }
    return 0;
}