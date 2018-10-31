#include "loader.h"
#include "file_system.h"
#include "console.h"
#include "dynamic_mem.h"
#include "../utils/util.h"

qword k_executeApp(const char* fileName, const char* args, byte affinity) {
	dword fileSize;
	Dir* dir;
	dirent* entry;
	byte* fileBuffer;
	File* file;
	qword appMemAddr;
	qword appMemSize;
	qword entryPointAddr;
	Task* task;

	/* get file size */
	fileSize = 0;
	dir = opendir("/");
	
	while (true) {
		entry = readdir(dir);
		if (entry == null) {
			break;
		}

		if (k_equalStr(entry->d_name, fileName) == true) {
			fileSize = entry->d_size;
			break;
		}
	}

	closedir(dir);

	if (fileSize == 0) {
		k_printf("loader error: %s does not exist or is zero-sized.\n", fileName);
		return TASK_INVALIDID;
	}

	/* read file */
	fileBuffer = (byte*)k_allocMem(fileSize);
	if (fileBuffer == null) {
		k_printf("loader error: file buffer allocation failure\n");
		return TASK_INVALIDID;
	}

	file = fopen(fileName, "r");
	if (file == null) {
		k_printf("loader error: %s opening failure\n", fileName);
		k_freeMem(fileBuffer);
		return TASK_INVALIDID;
	}

	if (fread(fileBuffer, 1, fileSize, file) != fileSize) {
		k_printf("loader error: %s reading failure\n", fileName);
		fclose(file);
		k_freeMem(fileBuffer);
		return TASK_INVALIDID;
	}

	fclose(file);

	/* load and relocate sections */
	if (k_loadSections(fileBuffer, &appMemAddr, &appMemSize, &entryPointAddr) == false) {
		k_printf("loader error: sections loading or relocation failure\n");
		k_freeMem(fileBuffer);
		return TASK_INVALIDID;
	}

	k_freeMem(fileBuffer);

	#if __DEBUG__
	k_printf("loader debug: execute '%s' with args: '%s'\n", fileName, args);
	#endif // __DEBUG__

	/* create task and add argument string to task */
	task = k_createTask(TASK_FLAGS_PROCESS | TASK_FLAGS_USER, (void*)appMemAddr, appMemSize, entryPointAddr, affinity);
	if (task == null) {
		k_printf("loader error: task creation failure\n");
		k_freeMem((void*)appMemAddr);
		return TASK_INVALIDID;
	}

	k_addArgsToTask(task, args);

	return task->link.id;
}

static bool k_loadSections(const byte* fileBuffer, qword* appMemAddr, qword* appMemSize, qword* entryPointAddr) {
	Elf64_Ehdr* eh;           // ELF header
	Elf64_Shdr* sh;           // section header
	//Elf64_Shdr* shstr_sh;     // section name table section header
	Elf64_Addr last_sh_addr;  // last section address
	Elf64_Xword last_sh_size; // last section size
	int i;
	qword memSize; // application memory size
	byte* memAddr; // application memory address

	/* analyze ELF header */
	eh = (Elf64_Ehdr*)fileBuffer;
	sh = (Elf64_Shdr*)(fileBuffer + eh->e_shoff);
	//shstr_sh = sh + eh->e_shstrndx;

	#if 0
	k_printf("*** ELF Header Info ***\n");
	k_printf("- magic number          : 0x%x %c%c%c\n", eh->e_ident[0], eh->e_ident[1], eh->e_ident[2], eh->e_ident[3]);
	k_printf("- file type             : %d\n", eh->e_type);
	k_printf("- program header offset : 0x%q\n", eh->e_phoff);
	k_printf("- section header offset : 0x%q\n", eh->e_shoff);
	k_printf("- program header size   : 0x%x\n", eh->e_phentsize);
	k_printf("- program header count  : %d\n", eh->e_phnum);
	k_printf("- section header size   : 0x%x\n", eh->e_shentsize);
	k_printf("- section header count  : %d\n", eh->e_shnum);
	k_printf("- section name table section header index : %d\n", eh->e_shstrndx);
	#endif

	if ((eh->e_ident[EI_MAG0] != ELFMAG0) ||
		(eh->e_ident[EI_MAG1] != ELFMAG1) ||
		(eh->e_ident[EI_MAG2] != ELFMAG2) ||
		(eh->e_ident[EI_MAG3] != ELFMAG3) ||
		(eh->e_ident[EI_CLASS] != ELFCLASS64) ||
		(eh->e_ident[EI_DATA] != ELFDATA2LSB) ||
		(eh->e_type != ET_REL)) {
		k_printf("loader error: invalid ELF file\n");
		return false;
	}

	/* get application memory size and address */
	last_sh_addr = 0;
	last_sh_size = 0;
	for (i = 0; i < eh->e_shnum; i++) {
		if ((sh[i].sh_flags & SHF_ALLOC) && (sh[i].sh_addr >= last_sh_addr)) {
			last_sh_addr = sh[i].sh_addr;
			last_sh_size = sh[i].sh_size;
		}
	}

	memSize = (last_sh_addr + last_sh_size + 0x1000 - 1) & 0xFFFFFFFFFFFFF000; // aligned with 0x1000 (4 KB)
	
	memAddr = (byte*)k_allocMem(memSize);
	if (memAddr == null) {
		k_printf("loader error: application memory allocation failure\n");
		return false;
	}

	#if 0
	k_printf("\n*** Application Memory Info ***\n");
	k_printf("- last section address       : 0x%q\n", last_sh_addr);
	k_printf("- last section size          : 0x%q\n", last_sh_size);
	k_printf("- application memory address : 0x%q\n", memAddr);
	k_printf("- application memory size    : 0x%q\n\n", memSize);
	#endif
	
	/* load sections */
	for (i = 1; i < eh->e_shnum; i++) { // skip index 0 (null section header)
		if (((sh[i].sh_flags & SHF_ALLOC) != SHF_ALLOC) || (sh[i].sh_size == 0)) {
			continue;
		}

		sh[i].sh_addr += (Elf64_Addr)memAddr;

		if (sh[i].sh_type == SHT_NOBITS) {
			k_memset((void*)sh[i].sh_addr, 0, sh[i].sh_size);

		} else {
			k_memcpy((void*)sh[i].sh_addr, fileBuffer + sh[i].sh_offset, sh[i].sh_size);
		}

		//k_printf("loader info: section %d loading: from file 0x%q to memory 0x%q, size 0x%q\n", i, sh[i].sh_offset, sh[i].sh_addr, sh[i].sh_size);
	}

	//k_printf("loader info: sections loading success\n");

	/* relocate sections */
	if (k_relocateSections(fileBuffer) == false) {
		k_printf("loader error: sections relocation failure\n");
		k_freeMem(memAddr);	
		return false;
	}

	//k_printf("loader info: sections relocation success\n");

	/* copy results */
	*appMemAddr = (qword)memAddr;
	*appMemSize = memSize;
	*entryPointAddr = (qword)memAddr + eh->e_entry;

	return true;
}

static bool k_relocateSections(const byte* fileBuffer) {
	Elf64_Ehdr* eh;        // ELF header
	Elf64_Shdr* sh;        // section header
	int i;                 // relocation section header index
	int j;                 // relocation entry index
	int sym_shndx;         // symbol table section header index
	int torel_shndx;       // to-relocate section header index
	int symdef_shndx;      // symbol-defined section header index
	Elf64_Addr r_offset;   // relocation offset
	Elf64_Xword r_info;    // relocation info
	Elf64_Sxword r_addend; // relocation addend
	Elf64_Sxword r_value;  // relocation value
	int r_size;            // relocation size (byte)
	Elf64_Sym* sym;        // symbol table entry
	Elf64_Rel* rel;        // relocation entry
	Elf64_Rela* rela;      // relocation-addend entry
	
	eh = (Elf64_Ehdr*)fileBuffer;
	sh = (Elf64_Shdr*)(fileBuffer + eh->e_shoff);

	/* process relocation using relocation section (SHT_REL, SHT_RELA) */
	for (i = 1; i < eh->e_shnum; i++) { // skip index 0 (null section header)
		if ((sh[i].sh_type != SHT_REL) && (sh[i].sh_type != SHT_RELA)) {
			continue;
		}

		sym_shndx = sh[i].sh_link;
		torel_shndx = sh[i].sh_info;

		// get first symbol table entry.
		sym = (Elf64_Sym*)(fileBuffer + sh[sym_shndx].sh_offset);

		/* process relocation entry in relocation section */
		for (j = 0; j < sh[i].sh_size; ) {
			if (sh[i].sh_type == SHT_REL) {
				// get j-th relocation entry.
				rel = (Elf64_Rel*)(fileBuffer + sh[i].sh_offset + j);

				r_offset = rel->r_offset;
				r_info = rel->r_info;
				r_addend = 0;

				j += sizeof(Elf64_Rel);

			} else {
				// get j-th relocation-addend entry.
				rela = (Elf64_Rela*)(fileBuffer + sh[i].sh_offset + j);

				r_offset = rela->r_offset;
				r_info = rela->r_info;
				r_addend = rela->r_addend;

				j += sizeof(Elf64_Rela);
			}

			symdef_shndx = sym[REL_SYMBOLINDEX(r_info)].st_shndx;

			if (symdef_shndx == SHN_ABS) {
				continue;

			} else if (symdef_shndx == SHN_COMMON) {
				k_printf("loader error: common symbol not supported\n");
				return false;
			}

			/* calculate relocation value (r_value) */
			switch (REL_TYPE(r_info)) {
			// r_value = S + A
			case R_X86_64_64:
			case R_X86_64_32:
			case R_X86_64_32S:
			case R_X86_64_16:
			case R_X86_64_8:				
				r_value = (sh[symdef_shndx].sh_addr + sym[REL_SYMBOLINDEX(r_info)].st_value) + r_addend;
				break;

			// r_value = S + A - P
			case R_X86_64_PC32:
			case R_X86_64_PC16:
			case R_X86_64_PC8:
			case R_X86_64_PC64:
				r_value = (sh[symdef_shndx].sh_addr + sym[REL_SYMBOLINDEX(r_info)].st_value) + r_addend - (sh[torel_shndx].sh_addr + r_offset);
				break;

			// r_value = B + A
			case R_X86_64_RELATIVE:
				r_value = sh[i].sh_addr + r_addend;
				break;

			// r_value = Z + A
			case R_X86_64_SIZE32:
			case R_X86_64_SIZE64:
				r_value = sym[REL_SYMBOLINDEX(r_info)].st_size + r_addend;
				break;

			default:
				k_printf("loader error: invalid relocation type: %d\n", REL_TYPE(r_info));
				return false;
			}

			/* calculate relocation size (r_size) */
			switch (REL_TYPE(r_info)) {
			// r_size = 64 bits
			case R_X86_64_64:
			case R_X86_64_RELATIVE:
			case R_X86_64_PC64:
			case R_X86_64_SIZE64:
				r_size = 8; // bytes
				break;

			// r_size = 32 bits
			case R_X86_64_PC32:
			case R_X86_64_32:
			case R_X86_64_32S:
			case R_X86_64_SIZE32:
				r_size = 4; // bytes
				break;

			// r_size = 16 bits
			case R_X86_64_16:
			case R_X86_64_PC16:
				r_size = 2; // bytes
				break;

			// r_size = 8 bits
			case R_X86_64_8:
			case R_X86_64_PC8:
				r_size = 1; // bytes
				break;

			default:
				k_printf("loader error: invalid relocation type: %d\n", REL_TYPE(r_info));
				return false;
			}

			/* apply relocation value and size to to-relocate section */
			switch (r_size) {
			case 8:
				*(Elf64_Sxword*)(sh[torel_shndx].sh_addr + r_offset) += r_value;
				break;

			case 4:
				*(int*)(sh[torel_shndx].sh_addr + r_offset) += (int)r_value;
				break;

			case 2:
				*(short*)(sh[torel_shndx].sh_addr + r_offset) += (short)r_value;
				break;

			case 1:
				*(char*)(sh[torel_shndx].sh_addr + r_offset) += (char)r_value;
				break;

			default:
				k_printf("loader error: invalid relocation size: %d bytes\n", r_size);
				return false;
			}
		}
	}

	return true;
}

static void k_addArgsToTask(Task* task, const char* args) {
	int len;
	int alignedLen;
	qword rsp; // new RSP address

	if (args == null) {
		len = 0;

	} else {
		len = k_strlen(args);
		if (len > LOADER_MAXARGSLENGTH) {
			len = LOADER_MAXARGSLENGTH;
		}
	}

	alignedLen = (len + 7) & 0xFFFFFFF8; // aligned with stack size (8 bytes).

	rsp = task->context.registers[TASK_INDEX_RSP] - (qword)alignedLen;
	k_memcpy((void*)rsp, args, len);
	*((byte*)rsp + len) = '\0';

	task->context.registers[TASK_INDEX_RSP] = rsp;
	task->context.registers[TASK_INDEX_RBP] = rsp;
	task->context.registers[TASK_INDEX_RDI] = rsp; // RDI (first parameter)
}
