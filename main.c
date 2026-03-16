#include <stdio.h>
#include <stdint.h>
#include <libdragon.h>

#define VI_BASE          0xA4400000
#define VI_CONTROL       (volatile uint32_t*)(VI_BASE + 0x00)
#define VI_V_CURRENT     (volatile uint32_t*)(VI_BASE + 0x10)
#define VI_H_SYNC        (volatile uint32_t*)(VI_BASE + 0x1C)
#define VI_H_SYNC_LEAP   (volatile uint32_t*)(VI_BASE + 0x20)

// MI_INTR_MASK_REG: 0xA430000C
// Writing 0x01 clears the VI interrupt bit (bit 3)
#define MI_INTR_MASK     (volatile uint32_t*)0xA430000C

int main(void) {
    // 1. Full init to ensure USB/Debug/Clocks are 100% functional
    debug_init_usblog();
    timer_init();
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);

    debugf("--- VI Manual Hammer (Console Active) ---\n");

    // 2. DISABLE LIBDRAGON'S VI HANDLER
    // This stops the library from resetting your registers during V-Blank
    *MI_INTR_MASK = 0x01; 

    uint32_t timings[64];

    while(1) {
        // 3. Set Extreme Values
        // Leap A = 1000 (Very short lines), Pattern 0 (Always use Leap A)
        uint32_t leap_val = (1000 << 16) | 3500; 
        uint32_t sync_val = (0x00 << 16) | 3093;

        *VI_H_SYNC_LEAP = leap_val;
        *VI_H_SYNC      = sync_val;

        // 4. Force a hardware latch wait
        // Wait for line 0, then wait to leave line 0
        while ((*VI_V_CURRENT >> 1) != 0);
        while ((*VI_V_CURRENT >> 1) == 0);

        disable_interrupts();
        uint32_t last_time = TICKS_READ();

        for (int i = 0; i < 64; i++) {
            uint32_t line = *VI_V_CURRENT >> 1;
            while ((*VI_V_CURRENT >> 1) == line);
            
            uint32_t now = TICKS_READ();
            timings[i] = now - last_time;
            last_time = now;
        }
        enable_interrupts();

        // 5. Verbose Output
        debugf("REG_H_SYNC: %08lX\n", *VI_H_SYNC);
        for (int i = 0; i < 64; i++) {
            debugf("%lu ", timings[i]);
            if((i+1) % 8 == 0) debugf("\n");
        }
        debugf("----------------------------\n");

        // Slow down the loop so we don't flood the USB buffer
        for(volatile int d=0; d<5000000; d++);
    }

    return 0;
}