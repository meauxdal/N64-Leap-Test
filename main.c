#include <stdio.h>
#include <stdint.h>
#include <libdragon.h>

#define VI_BASE          0xA4400000
#define VI_CONTROL       (volatile uint32_t*)(VI_BASE + 0x00)
#define VI_V_CURRENT     (volatile uint32_t*)(VI_BASE + 0x10)
#define VI_H_SYNC        (volatile uint32_t*)(VI_BASE + 0x1C)
#define VI_H_SYNC_LEAP   (volatile uint32_t*)(VI_BASE + 0x20)

int main(void) {
    debug_init_usblog();
    timer_init();
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);

    debugf("--- Pattern 0b11111 Test ---\n");

    // 1. Set distinct values: Leap A = 3500, Leap B = 1000
    // Pattern 0x1F (0b11111) = Lines 0-3 use Leap A, Line 4 uses Leap B
    uint32_t leap_val = (3500 << 16) | 1000;
    uint32_t sync_val = (0x1F << 16) | 3093;

    *VI_H_SYNC_LEAP = leap_val;
    *VI_H_SYNC      = sync_val;
    
    // Kick the control register to ensure the VI notices the change
    // We read the current state and write it back to trigger a latch update
    *VI_CONTROL = *VI_CONTROL;

    debugf("REG_H_SYNC:      %08lX\n", *VI_H_SYNC);
    debugf("REG_H_SYNC_LEAP: %08lX\n", *VI_H_SYNC_LEAP);

    // 2. Strong Latch: Wait for TWO field starts to be absolutely sure
    for(int i=0; i<2; i++) {
        while ((*VI_V_CURRENT >> 1) != 0); 
        while ((*VI_V_CURRENT >> 1) == 0);
    }

    disable_interrupts();
    uint32_t timings[16];
    
    // Sync to start of a line
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
