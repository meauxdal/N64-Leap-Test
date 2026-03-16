#include <stdio.h>
#include <stdint.h>
#include <libdragon.h>

#define VI_BASE          0xA4400000
#define VI_V_CURRENT     (volatile uint32_t*)(VI_BASE + 0x10)
#define VI_H_SYNC        (volatile uint32_t*)(VI_BASE + 0x1C)
#define VI_H_SYNC_LEAP   (volatile uint32_t*)(VI_BASE + 0x20)

int main(void) {
    debug_init_usblog();
    timer_init();
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);

    debugf("--- VI Leap Hardware Sync ---\n");

    // 1. Set distinct values: Leap A = 3500, Leap B = 4500
    // Pattern 0 = Always use Leap A
    uint32_t leap_val = (3500 << 16) | 4500;
    uint32_t sync_val = (0 << 16) | 3093;

    *VI_H_SYNC_LEAP = leap_val;
    *VI_H_SYNC      = sync_val;

    // Register Indicators (Verification)
    debugf("REG_H_SYNC:      %08lX\n", *VI_H_SYNC);
    debugf("REG_H_SYNC_LEAP: %08lX\n", *VI_H_SYNC_LEAP);

    // 2. The Latch: Wait for a full field transition
    // We wait for the line counter to hit 0 (V-Blank) 
    // This is the ONLY time the VI copies shadow regs to active regs.
    while ((*VI_V_CURRENT >> 1) != 0); 
    while ((*VI_V_CURRENT >> 1) == 0);

    // 3. Measurement
    disable_interrupts();
    uint32_t timings[16];
    
    // Sync to start of line 1
    uint32_t start_line = *VI_V_CURRENT >> 1;
    while ((*VI_V_CURRENT >> 1) == start_line);
    
    uint32_t last_time = TICKS_READ();

    for (int i = 0; i < 16; i++) {
        uint32_t current_line = *VI_V_CURRENT >> 1;
        while ((*VI_V_CURRENT >> 1) == current_line);
        
        uint32_t now = TICKS_READ();
        timings[i] = now - last_time;
        last_time = now;
    }
    enable_interrupts();

    for (int i = 0; i < 16; i++) {
        debugf("Line %02d: %lu ticks\n", i, timings[i]);
    }

    while (1);
    return 0;
}
