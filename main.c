#include <stdio.h>
#include <stdint.h>
#include <libdragon.h>

// Direct Hardware Addresses (Silicon Truth)
#define VI_BASE          0xA4400000
#define VI_H_SYNC        (VI_BASE + 0x1C)
#define VI_H_SYNC_LEAP   (VI_BASE + 0x20)
#define VI_V_CURRENT     (VI_BASE + 0x10)

// Function to wait for VBlank using direct register polling
void raw_vi_wait_vblank() {
    while (!(*(volatile uint32_t*)VI_V_CURRENT & 1));
}

int main(void) {
    // 1. Initialize Minimal Subsystems
    debug_init_usblog();
    timer_init();
    
    // We'll initialize the display just to get the clocks running, 
    // but we won't use it for rendering after the hijack.
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);

    debugf("Starting VI Leap Experiment...\n");

    // 2. Hardware Override
    // Base 3093 (Terminal Count for 3094)
    // LEAP_A: 4000
    // LEAP_B: 3093
    // Pattern: 0 (Forces LEAP_A every line)
    uint32_t leap_reg_val = (3093 << 16) | 4000;
    uint32_t sync_reg_val = (0 << 16) | 3093;

    // Sync to a fresh field before breaking the timing
    raw_vi_wait_vblank();

    *(volatile uint32_t*)VI_H_SYNC_LEAP = leap_reg_val;
    *(volatile uint32_t*)VI_H_SYNC      = sync_reg_val;

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

    // 4. Output Results
    debugf("\n--- Measurement Results ---\n");
    for (int i = 0; i < 16; i++) {
        uint32_t vi_cycles = (timings[i] * 4) / 3;
        debugf("Line %02d: %lu CPU Ticks (~%lu VI Cycles)\n", i, timings[i], vi_cycles);
    }

    // CRITICAL: Ensure the SC64 actually sees the data
    debug_flush(); 

    // 5. Hang here, but keep the USB active
    debugf("Experiment Complete. Holding...\n");
    debug_flush();

    while (1) {
        // Optional: blink an LED or just spin
        // Some SC64 firmware versions prefer the CPU stays active 
        // to handle USB handshakes depending on the libdragon version.
    }

    return 0;
}