// vim: ts=4 sw=4 expandtab
#ifndef __CPUSETMANAGER_H__
#define __CPUSETMANAGER_H__

#include <sysutils/NetlinkEvent.h>

class CpusetManager {
private:
    static CpusetManager *sInstance;
    CpusetManager();

    bool isOnline(int num);
    uint8_t mMaxCpus;
    uint64_t mCpuMask;
    int setCpuset(const char *filename, int cpus);
    void updateCpuset();

public:
    virtual ~CpusetManager();
    int start();
    int stop();
    void handleCpuEvent(NetlinkEvent *event);

    static CpusetManager *Instance();
};

#endif
