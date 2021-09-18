#pragma once

typedef char              int8_t;
typedef unsigned char     uint8_t;
typedef short             int16_t;
typedef unsigned short    uint16_t;
typedef int               int32_t;
typedef unsigned int      uint32_t;
typedef long int          int64_t;
typedef unsigned long int uint64_t;
typedef unsigned long int uintptr_t;

typedef enum {
    efi_reserved_memory_type,
    efi_loader_code,
    efi_loader_data,
    efi_boot_services_code,
    efi_boot_services_data,
    efi_runtime_services_code,
    efi_runtime_services_data,
    efi_conventional_memory,
    efi_unusable_memory,
    efi_ACPI_reclaim_memory,
    efi_ACPI_memory_NVS,
    efi_memory_mapped_IO,
    efi_memory_mapped_IO_port_space,
    efi_pal_code
} memory_type_t;

typedef struct {
    uint8_t magic[2]; // Magic number
    uint8_t mode;     // PSF font mode
    uint8_t charsize; // Character size
} psf1_header_t;

typedef struct {
    psf1_header_t psf1_header;
    char          glyph_buffer[512 * 16];
} psf1_font_t;

typedef struct {
    uint32_t *base;
    uint32_t  width;
    uint32_t  height;
    uint32_t  pixel_per_scan_line;
} framebuffer_t;

typedef struct {
    uint32_t type;
    uint32_t pad;
    void *   paddr;
    void *   vaddr;
    uint64_t num_of_pages;
    uint64_t attribute;
} memory_descriptor_t;

typedef struct {
    memory_descriptor_t *base;
    uint64_t             memory_map_size;
} memory_map_t;

typedef struct {
    framebuffer_t framebuffer;
    memory_map_t  memory_map;
    psf1_font_t   psf1_font;
    int           argc;
    char **       argv;
} bootparam_t;
