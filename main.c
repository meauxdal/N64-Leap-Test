#include <stdio.h>
#include <stdint.h>
#include <libdragon.h>

// Direct Hardware Addresses (Silicon Truth)
#define VI_BASE          0xA4400000
#define VI_H_SYNC        (VI_BASE + 0x1C)
#define VI_H_SYNC_LEAP   (VI_BASE + 0x20)
#define VI_V_CURRENT     (VI_BASE + 0x10)

// Function to wait for the next VI line transition
void raw_vi_wait_vblank() {
    uint32_t line = (*(volatile uint32_t*)VI_V_CURRENT) >> 1;

    // Wait until the line counter changes
    while (((*(volatile uint32_t*)VI_V_CURRENT) >> 1) == line) {
        /* spin */
    }
}

int main(void) {
    // 1. Initialize Minimal Subsystems
    debug_init_usblog();
    timer_init();
    
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);

    debugf("Starting VI Leap Experiment...\n");

    // 2. Hardware Override    
    uint32_t leap_reg_val = (3093 << 16) | 3193;
    uint32_t sync_reg_val = (0 << 16) | 3093;

    // Sync to a fresh field before breaking the timing
    raw_vi_wait_vblank();

    *(volatile uint32_t*)VI_H_SYNC_LEAP = leap_reg_val;
    *(volatile uint32_t*)VI_H_SYNC      = sync_reg_val;

    debugf("H_SYNC      = %08lx\n", *(volatile uint32_t*)VI_H_SYNC);
    debugf("H_SYNC_LEAP = %08lx\n", *(volatile uint32_t*)VI_H_SYNC_LEAP);

    // 3. Critical Measurement Section
    disable_interrupts(); 
    
    uint32_t timings[16];
    uint32_t last_time = TICKS_READ();

    for (int i = 0; i < 16; i++) {
        uint32_t current_line = (*(volatile uint32_t*)VI_V_CURRENT) >> 1;
        
        // Wait for line transition
        while (((*(volatile uint32_t*)VI_V_CURRENT) >> 1) == current_line) {
            /* busy wait */
        }
        
        uint32_t now = TICKS_READ();
        timings[i] = now - last_time;
        last_time = now;
    }

    enable_interrupts();

    // 4. Reporting results
    debugf("\n--- Measurement Results ---\n");
    for (int i = 0; i < 16; i++) {
        // CPU (46.875MHz) to VI (62.5MHz) Conversion: multiply by 4/3
        uint32_t vi_cycles = (timings[i] * 4) / 3;
        debugf("Line %02d: %lu CPU Ticks (~%lu VI Cycles)\n", i, timings[i], vi_cycles);
    }

    debugf("Experiment Complete. Waiting for USB sync...\n");

    wait_ms(2000); 

    while (1) {
    }

    return 0;
}