#include <bootparam.h>
#include <elf.h>
#include <uefi.h>
#include <utils.h>

typedef void (*__attribute__((sysv_abi)) kernel_entry_t)(bootparam_t *);

int main(int argc, char **argv) {
    // load the kernel
    char *buffer = NULL;
    if(load_kernel("kernel.elf", &buffer)) {
        return 1;
    }

    psf1_font_t psf1_font;
    if(load_font("zap-ext-light16.psf", &psf1_font)) {
        return 1;
    }

    // is it a valid ELF executable for this architecture?
    kernel_entry_t entry;
    Elf64_Ehdr *   elf = (Elf64_Ehdr *)buffer;
    if(!memcmp(elf->e_ident, ELFMAG, SELFMAG) && // magic match?
       elf->e_ident[EI_CLASS] == ELFCLASS64 &&   // 64 bit?
       elf->e_ident[EI_DATA] == ELFDATA2LSB &&   // LSB?
       elf->e_type == ET_EXEC &&                 // executable object?
       elf->e_machine == EM_MACH &&              // architecture match?
       elf->e_phnum > 0) {                       // has program headers?
        // load segments
        Elf64_Phdr *phdr = (Elf64_Phdr *)(buffer + elf->e_phoff);
        for(int i = 0; i < elf->e_phnum; i++) {
            if(phdr->p_type == PT_LOAD) {
                memcpy((void *)phdr->p_vaddr, buffer + phdr->p_offset,
                       phdr->p_filesz);
                memset((void *)(phdr->p_vaddr + phdr->p_filesz), 0,
                       phdr->p_memsz - phdr->p_filesz);
            }
            phdr = (Elf64_Phdr *)((uint8_t *)phdr + elf->e_phentsize);
        }
        entry = (kernel_entry_t)elf->e_entry;
    } else {
        printf("not a valid ELF executable for this architecture\n");
        return 0;
    }

    // setup gop mode
    efi_gop_t *gop = NULL;
    if(get_graphic_output_protocol(&gop)) {
        return 1;
    }
    int mode_of_max_resolution = 0;
    for(int i = 0, max_resolution = 0; i < gop->Mode->MaxMode; i++) {
        uintn_t              isiz   = sizeof(efi_gop_mode_info_t);
        efi_gop_mode_info_t *info   = NULL;
        efi_status_t         status = gop->QueryMode(gop, i, &isiz, &info);
        int                  resolution =
            info->HorizontalResolution * info->VerticalResolution;

        if(EFI_ERROR(status) || info->PixelFormat > PixelBitMask) {
            continue;
        }
        if(max_resolution < resolution) {
            mode_of_max_resolution = i;
            max_resolution         = resolution;
        }
    }
    gop->SetMode(gop, mode_of_max_resolution);

    // get the memory map
    efi_memory_descriptor_t *memory_map;
    uintn_t                  memory_map_size = 0, desc_size = 0;
    if(get_memory_map(&memory_map, &memory_map_size, &desc_size)) {
        return 1;
    }

    // set up boot parameters passed to the "kernel"
    bootparam_t bootp;
    memset(&bootp, 0, sizeof(bootparam_t));
    parse_args(&bootp, argc, argv);
    bootp.psf1_font         = psf1_font;
    bootp.framebuffer.base  = (uint32_t *)gop->Mode->FrameBufferBase;
    bootp.framebuffer.width = gop->Mode->Information->HorizontalResolution;
    bootp.framebuffer.height = gop->Mode->Information->VerticalResolution;
    bootp.framebuffer.pixel_per_scan_line =
        gop->Mode->Information->PixelsPerScanLine;
    bootp.memory_map.memory_map_size = memory_map_size / desc_size;
    bootp.memory_map.base = malloc(bootp.memory_map.memory_map_size
                                   * sizeof(memory_descriptor_t));
    for(int i = 0; i < bootp.memory_map.memory_map_size; i++) {
        efi_memory_descriptor_t *desc =
            (efi_memory_descriptor_t *)((uint8_t *)memory_map
                                        + i * desc_size);
        bootp.memory_map.base[i].type  = desc->Type;
        bootp.memory_map.base[i].pad   = desc->Pad;
        bootp.memory_map.base[i].paddr = (void *)desc->PhysicalStart;
        bootp.memory_map.base[i].vaddr = (void *)desc->VirtualStart;
        bootp.memory_map.base[i].num_of_pages = desc->NumberOfPages;
        bootp.memory_map.base[i].attribute    = desc->Attribute;
    }

    // free resources
    free_kernel(buffer);
    free_memory_map(memory_map);

    // exit this UEFI bullshit
    if(exit_bs()) {
        printf("Ph'nglui mglw'nafh Chtulu R'lyeh wgah'nagl fhtagn\n"
               "(Hastur has a hold on us and won't let us go)\n");
        return 0;
    }

    // execute the "kernel"
    entry(&bootp);

    // failsafe, should never return just in case
    while(1) {
    }

    return 0;
}
