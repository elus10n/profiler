// test_perf_detailed.cpp
#include <iostream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <cstring>
#include <errno.h>
#include <vector>
#include <cstdint>

static long perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu, int group_fd, unsigned long flags) {
    return syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

struct EventTest {
    const char* name;
    uint32_t type;
    uint64_t config;
};

int main() 
{
    std::vector<EventTest> events = {
        {"CPU_CYCLES", PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES},
        {"INSTRUCTIONS", PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS},
        {"PAGE_FAULTS", PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS},
        {"CONTEXT_SWITCHES", PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CONTEXT_SWITCHES}
    };

    std::cout << "Testing perf events on current process (pid=" << getpid() << ")..." << std::endl;
    std::cout << "perf_event_paranoid: " << std::flush;
    system("cat /proc/sys/kernel/perf_event_paranoid");
    std::cout << std::endl;

    for (const auto& event : events) {
        struct perf_event_attr attr;
        memset(&attr, 0, sizeof(attr));
        attr.size = sizeof(attr);
        attr.type = event.type;
        attr.config = event.config;
        attr.disabled = 1;
        attr.exclude_kernel = 0;
        attr.exclude_hv = 1;

        std::cout << "Testing " << event.name << " (" << event.type << "." << event.config << ")... " << std::flush;
        
        int fd = perf_event_open(&attr, getpid(), -1, -1, 0);
        if (fd < 0) {
            std::cout << "FAILED: " << strerror(errno) << " (errno=" << errno << ")" << std::endl;
        } else {
            std::cout << "SUCCESS (fd=" << fd << ")" << std::endl;
            
            // Тестируем чтение
            uint64_t value;
            if (read(fd, &value, sizeof(value)) == sizeof(value)) {
                std::cout << "  Read value: " << value << std::endl;
            } else {
                std::cout << "  Read failed: " << strerror(errno) << std::endl;
            }
            
            close(fd);
        }
    }

    return 0;
}