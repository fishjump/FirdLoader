#pragma once

#include <bootparam.h>
#include <elf.h>
#include <uefi.h>

void parse_args(bootparam_t *pointer_of_bootparam, int argc, char **argv);

int get_graphic_output_protocol(efi_gop_t **pointer_of_gop);

int  load_kernel(const char *file, char **pointer_of_buffer);
void free_kernel(char *buffer);

int  get_memory_map(efi_memory_descriptor_t **pointer_of_memory_map,
                    uintn_t *                 pointer_of_memory_map_size,
                    uintn_t *                 pointer_of_desc_size);
void free_memory_map(efi_memory_descriptor_t *memory_map);
void print_memory_map(const efi_memory_descriptor_t const *memory_map,
                      const uintn_t                        memory_map_size,
                      const uintn_t                        desc_size);

int  load_font(const char *file, psf1_font_t *pointer_of_font);
void free_font(psf1_font_t font);