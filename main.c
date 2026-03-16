#include <stdio.h>
#include <stdint.h>
#include <libdragon.h>

#define VI_BASE          0xA4400000
#define VI_CONTROL       (volatile uint32_t*)(VI_BASE + 0x00)
#define VI_V_SYNC        (volatile uint32_t*)(VI_BASE + 0x0C)
#define VI_V_CURRENT     (volatile uint32_t*)(VI_BASE + 0x10)
#define VI_H_SYNC        (volatile uint32_t*)(VI_BASE + 0x1C)
#define VI_H_SYNC_LEAP   (volatile uint32_t*)(VI_BASE + 0x20)

int main(void) {
    // Keep ONLY the essentials for communication
    debug_init_usblog();
    timer_init();

    // 1. Minimum VI Start
    // We set a basic 16-bit BPP mode but don't call the full init.
    // This ensures the VI clock is actually ticking.
    *VI_CONTROL = 0x00000002; 

    debugf("--- Bare Metal VI Test ---\n");

    // 2. Setup Loop
    // We will try to catch the latching point by writing repeatedly
    for(int attempt = 0; attempt < 100; attempt++) {
        
        // Use an unmistakable H-Sync: 1500 (very short lines)
        uint32_t leap_val = (1500 << 16) | 1500; 
        uint32_t sync_val = (0x1F << 16) | 1500; 

        *VI_H_SYNC_LEAP = leap_val;
        *VI_H_SYNC      = sync_val;
        *VI_V_SYNC      = 0x20D; // Required to define field height

        // Every 10 attempts, wait for a V-Blank transition
        if (attempt % 10 == 0) {
            uint32_t line = *VI_V_CURRENT >> 1;
            while ((*VI_V_CURRENT >> 1) == line);
        }
    }

    debugf("Final Reg State: H:%08lX L:%08lX\n", *VI_H_SYNC, *VI_H_SYNC_LEAP);

    // 3. Long Duration Measurement
    disable_interrupts();
    
    // Increase array size to see more lines over time
    uint32_t timings[64]; 
    uint32_t last_time = TICKS_READ();

    for (int i = 0; i < 64; i++) {
        uint32_t current_line = *VI_V_CURRENT >> 1;
        while ((*VI_V_CURRENT >> 1) == current_line);
        
        uint32_t now = TICKS_READ();
        timings[i] = now - last_time;
        last_time = now;
    }
    enable_interrupts();

    // 4. Output Results
    for (int i = 0; i < 64; i++) {
        debugf("L%02d: %lu\n", i, timings[i]);
    }

    debugf("Experiment Complete. Spinning...\n");
    while (1) {
        // Keep writing in the background to prevent any auto-reset
        *VI_H_SYNC_LEAP = (1500 << 16) | 1500;
        *VI_H_SYNC = (0x1F << 16) | 1500;
    }
    
    return 0;
}
