#ifndef METRIC_TYPES_H
#define METRIC_TYPES_H

enum class MetricType 
{
    INSTRUCTIONS,      
    CPU_CYCLES,        
    CACHE_MISSES,      
    CACHE_REFERENCES,  
    BRANCH_MISSES,     
    PAGE_FAULTS,      
    CONTEXT_SWITCHES   
};

#endif