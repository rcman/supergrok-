.section .data
    fb_path:    .asciz "/dev/fb0"           // Framebuffer device path
    open_mode:  .quad 2                     // O_RDWR flag for open (2)
    rect_color: .word 0xFF0000              // Red color in RGB (32-bit, ignoring alpha)

.section .bss
    fb_fd:      .skip 8                     // File descriptor for framebuffer
    fb_addr:    .skip 8                     // Address of mapped framebuffer
    fb_size:    .skip 8                     // Size of framebuffer in bytes
    fb_info:    .skip 64                    // Space for framebuffer info (simplified)

.section .text
.global _start

_start:
    // Step 1: Open /dev/fb0
    mov x8, #2                  // Syscall number for openat (AT_FDCWD not needed in simple case)
    adrp x0, fb_path            // Load framebuffer path
    add x0, x0, :lo12:fb_path
    ldr x1, open_mode           // O_RDWR mode
    mov x2, #0                  // No special permissions
    svc #0                      // Make syscall
    str x0, [fb_fd]             // Store file descriptor
    cmp x0, #0                  // Check for error
    blt exit                    // Exit if failed

    // Step 2: Get framebuffer info (simplified, assume 1920x1080, 32bpp)
    mov x8, #54                 // Syscall number for ioctl
    ldr x0, [fb_fd]             // File descriptor
    mov x1, #0x4600             // FBIOGET_VSCREENINFO (simplified)
    adrp x2, fb_info            // Buffer for info
    add x2, x2, :lo12:fb_info
    svc #0                      // Make syscall
    cmp x0, #0                  // Check for error
    blt close_fb                // Close and exit if failed

    // Calculate framebuffer size (assume 1920x1080x4 bytes)
    mov x0, #1920               // Width
    mov x1, #1080               // Height
    mov x2, #4                  // Bytes per pixel (32-bit)
    mul x0, x0, x1              // Width * Height
    mul x0, x0, x2              // Total bytes
    str x0, [fb_size]           // Store size

    // Step 3: Map framebuffer to memory
    mov x8, #192                // Syscall number for mmap
    mov x0, #0                  // Let kernel choose address
    ldr x1, [fb_size]           // Length to map
    mov x2, #3                  // PROT_READ | PROT_WRITE
    mov x3, #1                  // MAP_SHARED
    ldr x4, [fb_fd]             // File descriptor
    mov x5, #0                  // Offset
    svc #0                      // Make syscall
    str x0, [fb_addr]           // Store mapped address
    cmp x0, #-1                 // Check for error
    beq close_fb                // Close and exit if failed

    // Step 4: Draw a 100x100 red rectangle at (100,100)
    ldr x0, [fb_addr]           // Base address of framebuffer
    mov x1, #1920               // Screen width (assumed)
    mov x2, #4                  // Bytes per pixel
    mov x3, #100                // X start
    mov x4, #100                // Y start
    mov x5, #100                // Width of rectangle
    mov x6, #100                // Height of rectangle
    ldr w7, rect_color          // Red color

    // Calculate starting offset: (y * width + x) * bytes_per_pixel
draw_loop_y:
    mov x8, x4                  // Current Y
    mul x9, x8, x1              // Y * width
    add x9, x9, x3              // + X
    mul x9, x9, x2              // * bytes per pixel
    add x10, x0, x9             // Address to write

    mov x11, x5                 // Width counter
draw_loop_x:
    str w7, [x10], #4           // Write red pixel, increment address
    subs x11, x11, #1           // Decrease width counter
    bne draw_loop_x             // Loop until width done

    add x4, x4, #1              // Next Y line
    subs x6, x6, #1             // Decrease height counter
    bne draw_loop_y             // Loop until height done

    // Step 5: Clean up
    mov x8, #91                 // Syscall number for munmap
    ldr x0, [fb_addr]           // Address to unmap
    ldr x1, [fb_size]           // Length to unmap
    svc #0                      // Make syscall

close_fb:
    mov x8, #3                  // Syscall number for close
    ldr x0, [fb_fd]             // File descriptor
    svc #0                      // Make syscall

exit:
    mov x8, #93                 // Syscall number for exit
    mov x0, #0                  // Exit code 0
    svc #0                      // Make syscall
