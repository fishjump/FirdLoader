#include <utils.h>

#define PSF1_MAG "\x36\x04"
#define PSF1_MAG_LEN 2
#define PSF1_MODE512 0x01
#define PSF1_MODEHASTAB 0x02
#define PSF1_MODEHASSEQ 0x04
#define PSF1_MAXMODE 0x05

void parse_args(bootparam_t *pointer_of_bootparam, int argc, char **argv) {
    if(argc > 1) {
        pointer_of_bootparam->argc = argc - 1;
        pointer_of_bootparam->argv =
            (char **)malloc(argc * sizeof(char *));
        if(pointer_of_bootparam->argv) {
            int i = 0;
            for(i = 0; i < pointer_of_bootparam->argc; i++) {
                if((pointer_of_bootparam->argv[i] =
                        (char *)malloc(strlen(argv[i + 1]) + 1))) {
                    strcpy(pointer_of_bootparam->argv[i], argv[i + 1]);
                }
            }
            pointer_of_bootparam->argv[i] = NULL;
        }
    }
}

int load_kernel(const char *file, char **pointer_of_buffer) {
    FILE *f;
    if(f = fopen(file, "r")) {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        *pointer_of_buffer = malloc(size + 1);
        if(!(*pointer_of_buffer)) {
            printf("Unable to allocate memory\n");
            return 1;
        }
        fread(*pointer_of_buffer, size, 1, f);
        fclose(f);
    } else {
        printf("Unable to open file\n");
        return 1;
    }
    return 0;
}

void free_kernel(char *buffer) {
    free(buffer);
}

int get_graphic_output_protocol(efi_gop_t **pointer_of_gop) {
    efi_guid_t   gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    efi_status_t status =
        BS->LocateProtocol(&gopGuid, NULL, (void **)pointer_of_gop);
    if(!EFI_ERROR(status) && *pointer_of_gop) {
        status = (*pointer_of_gop)->SetMode((*pointer_of_gop), 0);
        ST->ConOut->Reset(ST->ConOut, 0);
        ST->StdErr->Reset(ST->StdErr, 0);
        if(EFI_ERROR(status)) {
            printf("Unable to set video mode\n");
            return 1;
        }
    } else {
        printf("Unable to get graphics output protocol\n");
        return 1;
    }

    return 0;
}

int get_memory_map(efi_memory_descriptor_t **pointer_of_memory_map,
                   uintn_t *                 pointer_of_memory_map_size,
                   uintn_t *                 pointer_of_desc_size) {

    efi_memory_descriptor_t *mement;
    uintn_t                  map_key = 0;

    // get the memory map
    efi_status_t status =
        BS->GetMemoryMap(pointer_of_memory_map_size, NULL, &map_key,
                         pointer_of_desc_size, NULL);
    if(status != EFI_BUFFER_TOO_SMALL || !(*pointer_of_memory_map_size)) {
        printf("Unable to get memory map\n");
        return 1;
    }

    // in worst case malloc allocates two blocks, and each block might
    // split a record into three, that's 4 additional records
    *pointer_of_memory_map_size += 4 * (*pointer_of_desc_size);
    *pointer_of_memory_map =
        (efi_memory_descriptor_t *)malloc(*pointer_of_memory_map_size);
    if(!*pointer_of_memory_map) {
        printf("Unable to allocate memory\n");
        return 1;
    }

    // get the memory map
    status = BS->GetMemoryMap(pointer_of_memory_map_size,
                              *pointer_of_memory_map, &map_key,
                              pointer_of_desc_size, NULL);
    if(EFI_ERROR(status)) {
        printf("Unable to get memory map\n");
        return 1;
    }

    return 0;
}

void free_memory_map(efi_memory_descriptor_t *memory_map) {
    free(memory_map);
}

void print_memory_map(const efi_memory_descriptor_t const *memory_map,
                      const uintn_t                        memory_map_size,
                      const uintn_t                        desc_size) {
    const char *types[] = {"EfiReservedMemoryType",
                           "EfiLoaderCode",
                           "EfiLoaderData",
                           "EfiBootServicesCode",
                           "EfiBootServicesData",
                           "EfiRuntimeServicesCode",
                           "EfiRuntimeServicesData",
                           "EfiConventionalMemory",
                           "EfiUnusableMemory",
                           "EfiACPIReclaimMemory",
                           "EfiACPIMemoryNVS",
                           "EfiMemoryMappedIO",
                           "EfiMemoryMappedIOPortSpace",
                           "EfiPalCode"};

    printf("Address              Size Type\n");
    for(efi_memory_descriptor_t const *mement = memory_map;
        (uint8_t *)mement < (uint8_t *)memory_map + memory_map_size;
        mement = NextMemoryDescriptor(mement, desc_size)) {
        printf("%016x %8d %02x %s\n", mement->PhysicalStart,
               mement->NumberOfPages, mement->Type, types[mement->Type]);
    }
}

int load_font(const char *file, psf1_font_t *pointer_of_font) {
    FILE *f;
    if(!(f = fopen(file, "r"))) {
        printf("Unable to open file\n");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    pointer_of_font->psf1_header = malloc(sizeof(psf1_header_t));
    if(!pointer_of_font->psf1_header) {
        printf("Unable to allocate memory\n");
        return 1;
    }
    fread(pointer_of_font->psf1_header, sizeof(psf1_header_t), 1, f);

    if(memcmp(pointer_of_font->psf1_header->magic, PSF1_MAG,
              PSF1_MAG_LEN)) {
        printf("not a valid PSF file, reason: wrong magic number\n");
        return 1;
    }

    uint32_t glyph_buffer_size =
        (pointer_of_font->psf1_header->mode & PSF1_MODE512 == 0 ? 256
                                                                : 512)
        * pointer_of_font->psf1_header->charsize;
    if(size < sizeof(psf1_header_t) + glyph_buffer_size) {
        printf("not a valid PSF file, reason: header and data size is out "
               "of file size\n");
        return 1;
    }
    pointer_of_font->glyph_buffer = malloc(glyph_buffer_size);
    fread(pointer_of_font->glyph_buffer, glyph_buffer_size, 1, f);

    fclose(f);
    return 0;
}

void free_font(psf1_font_t font) {
    free(font.psf1_header);
    free(font.glyph_buffer);
}