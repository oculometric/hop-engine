#include "engine.h"

#if defined(_WIN32)

using namespace HopEngine;

float Engine::getCPUUsagePercent() { return 0.0f; }
float Engine::getMemoryUsageMegabytes() { return 0.0f; }

#else

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/times.h"

using namespace HopEngine;

float Engine::getCPUUsagePercent()
{
    static double last_cpu_time = 0.0f;
    static double last_tot_time = 0.0f;
    static double last_pct = 0.0f;

    timespec ts{};
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    double cpu_time = ts.tv_sec + ts.tv_nsec / 1000000000.0f;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    double tot_time = ts.tv_sec + ts.tv_nsec / 1000000000.0f;

    if ((tot_time - last_tot_time) < 0.1f)
        return last_pct;

    last_pct = (cpu_time - last_cpu_time) / (tot_time - last_tot_time);

    last_cpu_time = cpu_time;
    last_tot_time = tot_time;

    return last_pct;
}

float Engine::getMemoryUsageMegabytes()
{
    FILE* file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];
    while (fgets(line, 128, file) != NULL)
    {
        if (strncmp(line, "VmSize:", 7) == 0)
        {
            int i         = strlen(line);
            const char* p = line;
            while (*p < '0' || *p > '9') ++p;
            line[i - 3] = '\0';
            result      = atoi(p);
            break;
        }
    }
    fclose(file);

    return static_cast<float>(result) / 1024.0f;
}

#endif

float HopEngine::Engine::getGPUUsagePercent() { return 0.0f; }

float HopEngine::Engine::getGPUMemoryUsageMegabytes() { return 0.0f; }
