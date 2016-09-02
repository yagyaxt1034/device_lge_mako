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
    mMaxCpuNum = sysconf(_SC_NPROCESSORS_CONF);
    mOnlineCpus.reserve(mMaxCpuNum);
}

CpusetManager::~CpusetManager() {
}

int CpusetManager::start() {
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

void CpusetManager::setCpuset(const char *filename, int cpus) {
    int fd = open(filename, O_WRONLY);
    if (fd < 0) {
        SLOGE("open %s failed: %s", filename, strerror(errno));
        return;
    }

    int online = mOnlineCpus.size();
    cpus = 1 > cpus ? 1 : cpus;
    cpus = online < cpus ? online : cpus;
    int pos = online - cpus;
    std::string cpumask;
    //for (int i = pos; i < online; i++) {
    for (int i = 0; i < cpus; i++) {
        android::base::StringAppendF(&cpumask, "%d,", mOnlineCpus[i]);
    }

    if (write(fd, cpumask.c_str(), cpumask.length()) < 0) {
        SLOGE("write %s to %s : failed: %s", cpumask.c_str(), filename, strerror(errno));
    }
    close(fd);
}

void CpusetManager::updateCpuset() {
    mOnlineCpus.clear();
    for (int cpu = 0; cpu < mMaxCpuNum; cpu++) {
        if (isOnline(cpu)) {
            mOnlineCpus.push_back(cpu);
        }
    }
    int cpus = mOnlineCpus.size();

    if (cpus == 0) {
        // never happens
    } else if (cpus == 1) {
        // do nothing
    } else {
        setCpuset(CPUSET_TOP_APP, cpus);
        setCpuset(CPUSET_FG, cpus *3/4 );
        setCpuset(CPUSET_FG_BOOST, cpus *3/4);
        setCpuset(CPUSET_BG_SYSTEM, cpus *3/4);
        setCpuset(CPUSET_BG, 1);
    }
}

void CpusetManager::handleCpuEvent(NetlinkEvent *event) {
    updateCpuset();
}
