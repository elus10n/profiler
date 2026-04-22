#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include <unistd.h>
#include <libelf.h>
#include <gelf.h>
#include <cxxabi.h>
#include <fcntl.h>

struct Symbol 
{
    uintptr_t addr;
    size_t size;
    std::string name;
};

class Demangler
{
    std::vector<Symbol> symbols;
    uintptr_t base_in_mem = 0;
    uintptr_t vaddr_offset = 0; 

    std::string demangle(const char* name) 
    {
        int status;
        char* res = abi::__cxa_demangle(name, 0, 0, &status);
        if (status == 0) 
        { 
            std::string s(res); 
            free(res); 
            return s; 
        }
        return name ? name : "???";
    }

    public:

    void load(const std::string& path, uintptr_t base) 
    {
        base_in_mem = base;
        int fd = open(path.c_str(), O_RDONLY);
        if (fd < 0) return;

        elf_version(EV_CURRENT);
        Elf* e = elf_begin(fd, ELF_C_READ, NULL);
        if (!e) 
        { 
            close(fd); 
            return; 
        }

        size_t phdrnum;
        elf_getphdrnum(e, &phdrnum);
        for (size_t i = 0; i < phdrnum; i++) 
        {
            GElf_Phdr phdr;
            gelf_getphdr(e, i, &phdr);
            if (phdr.p_type == PT_LOAD && (phdr.p_flags & PF_X)) 
            {
                vaddr_offset = phdr.p_vaddr;
                break;
            }
        }

        Elf_Scn* scn = NULL;
        while ((scn = elf_nextscn(e, scn))) 
        {
            GElf_Shdr sh; 
            gelf_getshdr(scn, &sh);

            if (sh.sh_type == SHT_SYMTAB || sh.sh_type == SHT_DYNSYM) 
            {
                Elf_Data* d = elf_getdata(scn, NULL);
                if (!d) continue;

                for (int i = 0; i < sh.sh_size / sh.sh_entsize; i++) 
                {
                    GElf_Sym s; 
                    gelf_getsym(d, i, &s);

                    if (GELF_ST_TYPE(s.st_info) == STT_FUNC && s.st_value > 0) 
                    {
                        const char* mangled_name = elf_strptr(e, sh.sh_link, s.st_name);
                        
                        if (mangled_name) 
                            symbols.push_back(Symbol{(uintptr_t)s.st_value, (size_t)s.st_size, demangle(mangled_name)});
                    }
                }
            }
        }

        auto comparator = [](const Symbol& a, const Symbol& b) {return a.addr < b.addr;};
        std::sort(symbols.begin(), symbols.end(), comparator);
        elf_end(e); 
        close(fd);
    }

    std::string find(uintptr_t ip) 
    {
        if (symbols.empty() || ip < base_in_mem) return "???";
        
        uintptr_t relative_ip = (ip - base_in_mem) + vaddr_offset;

        auto it = std::upper_bound(symbols.begin(), symbols.end(), relative_ip, [](uintptr_t val, const Symbol& s) { return val < s.addr; });
        
        if (it != symbols.begin()) 
        {
            auto res = --it;
            if (relative_ip >= res->addr && relative_ip < res->addr + res->size) return res->name;
        }
        return "???";
    }
};