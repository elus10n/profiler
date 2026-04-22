#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <map>
#include <memory>

#include "demangler.h"

class SymbolResolver 
{
    struct ModuleRegion 
    {
        uintptr_t start;
        uintptr_t end;
        std::unique_ptr<Demangler> demangler;
        std::string path;
    };

    std::vector<ModuleRegion> modules;
    pid_t target_pid;

public:
    SymbolResolver(pid_t pid) : target_pid(pid) {refresh();}

    std::string resolve(uintptr_t ip) const 
    {
        // 1. Ищем, в какой модуль попадает адрес (бинарный поиск по вектору)
        auto it = std::upper_bound(modules.begin(), modules.end(), ip, 
            [](uintptr_t val, const ModuleRegion& m) {return val < m.start;}
        );

        if (it != modules.begin()) 
        {
            const auto& region = *(--it);
            
            // Проверяем, что IP внутри границ региона
            if (ip >= region.start && ip < region.end) {return region.demangler->find(ip);}
        }

        return "???";
    }

    // Парсинг /proc/[pid]/maps для заполнения списка модулей
    void refresh() 
    {
        modules.clear();
        std::string maps_path = "/proc/" + std::to_string(target_pid) + "/maps";
        std::ifstream maps_file(maps_path);
        
        if (!maps_file.is_open()) return;

        std::string line;
        while (std::getline(maps_file, line)) 
        {
            // Формат строки: 00400000-00452000 r-xp 00000000 08:02 65536 /usr/bin/name
            std::istringstream iss(line);
            std::string addr_range, perms, offset, dev, inode, path;
            
            if (!(iss >> addr_range >> perms >> offset >> dev >> inode)) continue;
            
            // Нас интересуют только исполняемые регионы (флаг 'x')
            if (perms.find('x') == std::string::npos) continue;

            // Читаем путь (он может отсутствовать для анонимных маппингов)
            if (!(iss >> path) || path[0] == '[') continue;

            size_t dash = addr_range.find('-');
            uintptr_t start = std::stoull(addr_range.substr(0, dash), nullptr, 16);
            uintptr_t end = std::stoull(addr_range.substr(dash + 1), nullptr, 16);

            // Создаем деманглер для этого файла
            auto demangler = std::make_unique<Demangler>();
            demangler->load(path, start);

            modules.push_back({start, end, std::move(demangler), path});
        }

        // Сортируем на всякий случай, хотя /proc/maps обычно уже отсортирован
        auto comparator = [](const ModuleRegion& a, const ModuleRegion& b) {return a.start < b.start;};

        std::sort(modules.begin(), modules.end(), comparator);
    }
};