// vim: ts=4 sw=4 expandtab

#define LOG_NDEBUG 0
#define LOG_TAG "CpusetManager"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <cutils/log.h>
#include <cutils/fs.h>
#include <android-base/stringprintf.h>

#include "CpusetManager.h"

#define CPUSET_ROOT "/dev/cpuset/cpus"
#define CPUSET_FG "/dev/cpuset/foreground/cpus"
#define CPUSET_FG_BOOST "/dev/cpuset/foreground/boost/cpus"
#define CPUSET_BG "/dev/cpuset/background/cpus"
#define CPUSET_BG_SYSTEM "/dev/cpuset/system-background/cpus"
#define CPUSET_TOP_APP "/dev/cpuset/top-app/cpus"

#define SYSFS_CPU_ONLINE_FORMAT "/sys/devices/system/cpu/cpu%d/online"

CpusetManager::CpusetManager() {
    mMaxCpus = sysconf(_SC_NPROCESSORS_CONF);
}

CpusetManager::~CpusetManager() {
}

int CpusetManager::start() {
    if (mMaxCpus > 64) {
        SLOGE("Too many cpus, do not start mpcpusetd");
        return -1;
    }
    updateCpuset();
    return 0;
}

int CpusetManager::stop() {
    return 0;
}

CpusetManager* CpusetManager::sInstance = NULL;
CpusetManager* CpusetManager::Instance() {
    if (!sInstance) {
        sInstance = new CpusetManager();
    }
    return sInstance;
}

bool CpusetManager::isOnline(int num) {
    std::string sysfs_cpu_online(
            android::base::StringPrintf(SYSFS_CPU_ONLINE_FORMAT, num));
    int online = 0;
    fs_read_atomic_int(sysfs_cpu_online.c_str(), &online);
    return online == 1;
}

int CpusetManager::setCpuset(const char *filename, int cpus) {
    int fd = open(filename, O_WRONLY);
    if (fd < 0) {
        SLOGE("open %s failed: %s", filename, strerror(errno));
        return 0;
    }

    int busy = 0;
    std::string cpumask;

#define MAX(a, b) ((a) > (b) ? (a) : (b))
    cpus = MAX(1, cpus);
    int added = 0;

    for (int i = 0; i < mMaxCpus; i++) {
        if (mCpuMask & (1<<i)) {
            android::base::StringAppendF(&cpumask, "%d,", i);
            added++;
            if (cpus == added) {
                break;
            }
        }
    }

    if (write(fd, cpumask.c_str(), cpumask.length()) < 0) {
        if (errno == EBUSY) {
            busy = 1;
        } else {
            SLOGE("write %s to %s : failed: %s", cpumask.c_str(), filename, strerror(errno));
        }
    }
    close(fd);
    return busy;
}

void CpusetManager::updateCpuset() {
    uint64_t cpumask = 0;
    uint8_t online = 0;
    for (int cpu = 0; cpu < mMaxCpus; cpu++) {
        if (isOnline(cpu)) {
            cpumask |= 1<<cpu;
            online++;
        }
    }
    if (cpumask == mCpuMask) {
        return;
    }
    mCpuMask = cpumask;

    if (online == 0) {
        // never happens
    } else if (online == 1) {
        // do nothing
    } else {
        int busy;
        setCpuset(CPUSET_TOP_APP, online);
        busy = setCpuset(CPUSET_FG, online *3/4 );
        setCpuset(CPUSET_FG_BOOST, online *3/4);
        if (busy) {
            // XXX: try again
            setCpuset(CPUSET_FG, online *3/4 );
        }
        setCpuset(CPUSET_BG_SYSTEM, online *3/4);
        setCpuset(CPUSET_BG, 1);
    }
}

void CpusetManager::handleCpuEvent(NetlinkEvent *event) {
    updateCpuset();
}
