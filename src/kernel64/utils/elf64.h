#ifndef __UTILS_ELF64_H__
#define __UTILS_ELF64_H__

/**
  < ELF64 File Structure (2 types) >
    - hOS uses Relocatable File (ET_REL).

          Relocatable File (ET_REL)               Executable File (ET_EXEC)
    -------------------------------------   -------------------------------------
    | ELF header                        |   | ELF header                        |
    | # program header table (optional) |   | # program header table            |
    | section 0                         |   | segment header 0                  |
    | section 1                         |   | segment header 1                  |
    | section ...                       |   | segment header ...                |
    | section n                         |   | segment header n                  |
    | # section header table            |   | segment 0                         |
    | section header 0                  |   | segment 1                         |
    | section header 1                  |   | segment ...                       |
    | section header ...                |   | segment n                         |
    | section header n                  |   | # section header table (optional) |
    -------------------------------------   -------------------------------------

  < Section Name and Type >
    ----------------------------------------------------------------------------------
    |    Name    |     Type     |                    Description                     |
    ----------------------------------------------------------------------------------
    | .text      | SHT_PROGBITS | executable code section                            |
    | .data      | SHT_PROGBITS | initialized data section                           |
    | .rodata    | SHT_PROGBITS | read-only initialized data section                 |
    | .bss       | SHT_NOBITS   | not-initialized data section                       |
    |            |              | -> No data in file.                                |
    |            |              | -> Memory (sh_size) have to be allocated           |
    |            |              |    and initialized as 0 when loading this section. |
    | .shstrtab  | SHT_STRTAB   | section name table section                         |
    | .strtab    | SHT_STRTAB   | string table section                               |
    |            |              | -> String mainly means symbol                      |
    |            |              |    such as function name and global variable name. |
    | .symtab    | SHT_SYMTAB   | symbol table section                               |
    | .rel+name  | SHT_REL      | relocation section                                 |
    | .rela+name | SHT_RELA     | relocation-addend section                          |
    | .dynamic   | SHT_DYNAMIC  | dynamic linking section                            |
    ----------------------------------------------------------------------------------

  < Meaning of sh_link and sh_info by sh_type >
    -----------------------------------------------------------------------------------------------------------------------
    |     sh_type      |                        sh_link                         |                 sh_info                 |
    -----------------------------------------------------------------------------------------------------------------------
    | SHT_DYNAMIC      | string table section header index                      | 0                                       |
    | SHT_HASH         | to apply hash table, symbol table section header index | 0                                       |
    | SHT_REL          | symbol table section header index                      | to-relocate section header index        |
    | SHT_RELA         | same as above                                          | same as above                           |
    | SHT_SYMTAB       | string table section header index                      | last local symbol table entry index + 1 |
    | SHT_DYNSYM       | same as above                                          | same as above                           |
    | SHT_GROUP        | symbol table section header index                      | symbol table entry index                |
    | SHT_SYMTAB_SHNDX | symbol table entry-related section header index        | 0                                       |
    -----------------------------------------------------------------------------------------------------------------------

  < Diagram of sh_link and sh_info by sh_type >

    section header index          sh_link sh_info            
    ------------------------------------------
    | 0 | to-relocate section header | 0 | 0 |
    ------------------------------------------
      |
      --------------------------------------
                                           |
    ------------------------------------------
    | 1 | SHT_REL/A section header   | 2 | 0 |
    ------------------------------------------
                                       |
      ----------------------------------
      |
	------------------------------------------
    | 2 | SHT_SYMTAB section header  | 3 | 1 |
    ------------------------------------------ 
                                       |
      ----------------------------------
      |
    ------------------------------------------
    | 3 | SHT_STRTAB section header  | 0 | 0 |
    ------------------------------------------
*/

// ELF primitive data type
#define Elf64_Addr   unsigned long
#define Elf64_Off    unsigned long
#define Elf64_Half   unsigned short int
#define Elf64_Word   unsigned int
#define Elf64_Sword  int
#define Elf64_Xword  unsigned long
#define Elf64_Sxword long
#define Elf64_Lword  unsigned long

// e_ident[] index
#define EI_MAG0       0
#define EI_MAG1       1
#define EI_MAG2       2
#define EI_MAG3       3
#define EI_CLASS      4
#define EI_DATA       5
#define EI_VERSION    6
#define EI_OSABI      7
#define EI_ABIVERSION 8
#define EI_PAD        9  // padding (9 ~ 15)
#define EI_NIDENT     16

// e_ident[EI_MAGX]
#define ELFMAG0 0x7F
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

// e_ident[EI_CLASS]
#define ELFCLASSNONE 0
#define ELFCLASS32   1 // 32-bit file
#define ELFCLASS64   2 // 64-bit file

// e_ident[EI_DATA]
#define ELFDATANONE 0
#define ELFDATA2LSB 1 // little endian
#define ELFDATA2MSB 2 // big endian

// e_ident[EI_OSABI]
#define ELFOSABI_NONE    0
#define ELFOSABI_HPUX    1
#define ELFOSABI_NETBSD  2
#define ELFOSABI_LINUX   3
#define ELFOSABI_SOLARIS 6
#define ELFOSABI_AIX     7
#define ELFOSABI_FREEBSD 9

// e_type
#define ET_NONE   0
#define ET_REL    1 // relocatable file (used by hOS)
#define ET_EXEC   2 // executable file
#define ET_DYN    3
#define ET_CORE   4
#define ET_LOOS   0xFE00
#define ET_HIOS   0xFEFF
#define ET_LOPROC 0xFF00
#define ET_HIPROC 0xFFFF

// e_machine
#define EM_NONE   0
#define EM_M32    1
#define EM_SPARC  2
#define EM_386    3
#define EM_PPC    20
#define EM_PPC64  21
#define EM_ARM    40
#define EM_IA_64  50
#define EM_X86_64 62
#define EM_AVR    83
#define EM_AVR32  185
#define EM_CUDA   190

// special section header index
#define SHN_UNDEF     0
#define SHN_LORESERVE 0xFF00
#define SHN_LOPROC    0xFF00
#define SHN_HIPROC    0xFF1F
#define SHN_LOOS      0xFF20
#define SHN_HIOS      0xFF3F
#define SHN_ABS       0xFFF1
#define SHN_COMMON    0xFFF2
#define SHN_XINDEX    0xFFFF
#define SHN_HIRESERVE 0xFFFF

// sh_type
#define SHT_NULL          0
#define SHT_PROGBITS      1
#define SHT_SYMTAB        2
#define SHT_STRTAB        3
#define SHT_RELA          4
#define SHT_HASH          5
#define SHT_DYNAMIC       6
#define SHT_NOTE          7
#define SHT_NOBITS        8
#define SHT_REL           9
#define SHT_SHLIB         10
#define SHT_DYNSYM        11
#define SHT_INIT_ARRAY    14
#define SHT_FINI_ARRAY    15
#define SHT_PREINIT_ARRAY 16
#define SHT_GROUP         17
#define SHT_SYMTAB_SHNDX  18
#define SHT_LOOS          0x60000000
#define SHT_HIOS          0x6FFFFFFF
#define SHT_LOPROC        0x70000000
#define SHT_HIPROC        0x7FFFFFFF
#define SHT_LOUSER        0x80000000
#define SHT_HIUSER        0xFFFFFFFF

// sh_flags
#define SHF_WRITE     1
#define SHF_ALLOC     2
#define SHF_EXECINSTR 4
#define SHF_MASKOS    0x0FF00000
#define SHF_MASKPROC  0xF0000000

// st_info - symbol binding (high 4 bits)
#define STB_LOCAL  0
#define STB_GLOBAL 1
#define STB_WEAK   2
#define STB_LOOS   10
#define STB_HIOS   12
#define STB_LOPROC 13
#define STB_HIPROC 15

// st_info - symbol type (low 4 bits)
#define STT_NOTYPE  0
#define STT_OBJECT  1
#define STT_FUNC    2
#define STT_SECTION 3
#define STT_FILE    4
#define STT_COMMON  5
#define STT_LOOS    10
#define STT_HIOS    12
#define STT_LOPROC  13
#define STT_HIPROC  15

// r_info - relocation type (low 32 bits)
// @ relocation value (r_value)
// - S: symbol-defined section memory address + st_value
// - P: to-relocate section memory address + r_offset
// - A: r_addend
// - B: relocation section memory address
// - Z: st_size
                              // relocation size (r_size, bit) : relocation value (r_value)
#define R_X86_64_NONE      0  // none
#define R_X86_64_64        1  // 64 : S + A
#define R_X86_64_PC32      2  // 32 : S + A - P
#define R_X86_64_GOT32     3  // 32 : G + A
#define R_X86_64_PLT32     4  // 32 : L + A - P
#define R_X86_64_COPY      5  // none
#define R_X86_64_GLOB_DAT  6  // 64 : S
#define R_X86_64_JUMP_SLOT 7  // 64 : S
#define R_X86_64_RELATIVE  8  // 64 : B + A
#define R_X86_64_GOTPCREL  9  // 32 : G + GOT + A - P
#define R_X86_64_32        10 // 32 : S + A
#define R_X86_64_32S       11 // 32 : S + A
#define R_X86_64_16        12 // 16 : S + A
#define R_X86_64_PC16      13 // 16 : S + A - P
#define R_X86_64_8         14 // 8  : S + A
#define R_X86_64_PC8       15 // 8  : S + A - P
#define R_X86_64_DPTMOD64  16 // 64
#define R_X86_64_DTPOFF64  17 // 64
#define R_X86_64_TPOFF64   18 // 64
#define R_X86_64_TLSGD     19 // 32
#define R_X86_64_TLSLD     20 // 32
#define R_X86_64_DTPOFF32  21 // 32
#define R_X86_64_GOTTPOFF  22 // 32
#define R_X86_64_TPOFF32   23 // 32
#define R_X86_64_PC64      24 // 64 : S + A - P
#define R_X86_64_GOTOFF64  25 // 64 : S + A - GOT
#define R_X86_64_GOTPC32   26 // 32 : GOT + A - P
#define R_X86_64_SIZE32    32 // 32 : Z + A
#define R_X86_64_SIZE64    33 // 64 : Z + A

/* macro functions */
#define REL_SYMBOLINDEX(r_info) ((r_info) >> 32)        // get symbol index (high 32 bits) of r_info
#define REL_TYPE(r_info)        ((r_info) & 0xFFFFFFFF) // get relocation type (low 32 bits) of r_info

#pragma pack(push, 1)

// ELF header
typedef struct {
	unsigned char e_ident[EI_NIDENT]; // file identifier
	Elf64_Half    e_type;             // file type
	Elf64_Half    e_machine;          // machine architecture
	Elf64_Word    e_version;          // file version
	Elf64_Addr    e_entry;            // entry point (_start)
	Elf64_Off     e_phoff;            // first program header offset (program header table offset) in file
	Elf64_Off     e_shoff;            // first section header offset (section header table offset) in file
	Elf64_Word    e_flags;            // architecture-specific flags
	Elf64_Half    e_ehsize;           // ELF header size
	Elf64_Half    e_phentsize;        // program header size
	Elf64_Half    e_phnum;            // program header count
	Elf64_Half    e_shentsize;        // section header size
	Elf64_Half    e_shnum;            // section header count
	Elf64_Half    e_shstrndx;         // section name table section (.shstrtab) header index
} Elf64_Ehdr;

// program header
typedef struct {
	Elf64_Word  p_type;   // entry type
	Elf64_Word  p_flags;  // access permission flags
	Elf64_Off   p_offset; // file offset of contents
	Elf64_Addr  p_vaddr;  // virtual address in memory
	Elf64_Addr  p_paddr;  // physical address (not used)
	Elf64_Xword p_filesz; // size of contents in file
	Elf64_Xword p_memsz;  // size of contents in memory
	Elf64_Xword p_align;  // alignment in memory and file
} Elf64_Phdr;

// section header
typedef struct {
	Elf64_Word  sh_name;      // section name offset in section name table section (.shstrtab)
	                          // - current section name: e_shstrndx -> sh_offset + sh_name
	Elf64_Word  sh_type;      // section type
	Elf64_Xword sh_flags;     // section flags
	Elf64_Addr  sh_addr;      // section address in memory
	Elf64_Off   sh_offset;    // section offset in file
	Elf64_Xword sh_size;      // section size in memory
	                          // - It could be different from section size in file.
	Elf64_Word  sh_link;      // linked section header index (depend on section type)
	                          // - refer to < Meaning of sh_link and sh_info by sh_type >
	                          // - If sh_type == SHT_REL or SHT_RELA, sh_link means symbol table section header index.
	                          // - If sh_type == SHT_SYMTAB, sh_link means string table section header index.
	Elf64_Word  sh_info;      // additional info (depend on section type)
	                          // - refer to < Meaning of sh_link and sh_info by sh_type >
	                          // - If sh_type == SHT_REL or SHT_RELA, sh_info means to-relocate section header index.
	Elf64_Xword sh_addralign; // memory address alignment
	Elf64_Xword sh_entsize;   // data entry size in section
} Elf64_Shdr;

// symbol table entry (SHT_SYMTAB)
typedef struct {
	Elf64_Word    st_name;  // section name offset in symbol table section (.symtab)
	unsigned char st_info;	// symbol info
	                        // - symbol binding (high 4 bits) and symbol type (low 4 bits)
	unsigned char st_other;	// reserved (not used)
	Elf64_Half    st_shndx;	// symbol-defined section header index
	Elf64_Addr    st_value;	// symbol value
	                        // - If e_type == ET_REL, st_value means symbol offset in st_shndx section memory.
	                        // - If e_type == ET_EXEC, st_value means symbol memory address.
	Elf64_Xword   st_size;	// symbol size
	                        // - If symbol is variable, st_size means data size of variable.
	                        // - If symbol is function, st_size means code size in function.
} Elf64_Sym;

// relocation entry (SHT_REL)
typedef struct {
	Elf64_Addr  r_offset; // relocation offset in to-relocate section memory
	Elf64_Xword r_info;   // relocation info
	                      // - symbol index (high 32 bits) and relocation type (low 32 bits)
} Elf64_Rel;

// relocation-addend entry (SHT_RELA)
typedef struct {
	Elf64_Addr   r_offset; // relocation offset in to-relocate section memory
	Elf64_Xword  r_info;   // relocation info
	                       // - symbol index (high 32 bits) and relocation type (low 32 bits)
	Elf64_Sxword r_addend; // relocation addend
} Elf64_Rela;

#pragma pack(pop)

#endif // __UTILS_ELF64_H__