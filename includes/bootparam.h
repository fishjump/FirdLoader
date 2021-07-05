#pragma once

#include <uefi.h>

typedef struct {
    uint8_t magic[2]; // Magic number
    uint8_t mode;     // PSF font mode
    uint8_t charsize; // Character size
} psf1_header_t;

typedef struct {
    psf1_header_t *psf1_header;
    void *         glyph_buffer;
} psf1_font_t;

typedef struct {
    uint32_t *base;
    uint32_t  width;
    uint32_t  height;
    uint32_t  pixel_per_scan_line;
} framebuffer_t;

typedef struct {
    efi_memory_descriptor_t *base;
    uint64_t                 memory_map_size;
    uint64_t                 desc_size;
} memory_map_t;

typedef struct {
    framebuffer_t framebuffer;
    memory_map_t  memory_map;
    psf1_font_t   psf1_font;
    int           argc;
    char **       argv;
} bootparam_t;
