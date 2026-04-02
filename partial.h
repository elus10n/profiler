#ifndef PARTIAL_H
#define PARTIAL_H

#include <map>
#include <cstdint>
#include <memory>

using snapshotData = std::map<std::shared_ptr<Metric>, uint64_t>;

struct HotspotRawData 
{
    uint64_t head;         
    uint64_t tail;         
    uint8_t* data_start;    
    size_t data_mask;       
};

#endif