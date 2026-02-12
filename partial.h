#ifndef PARTIAL_H
#define PARTIAL_H

#include <map>
#include <cstdint>

using snapshotData = std::map<std::shared_ptr<Metric>, uint64_t>;

#endif