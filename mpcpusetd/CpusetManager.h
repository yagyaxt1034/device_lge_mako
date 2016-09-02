// vim: ts=4 sw=4 expandtab
#ifndef __CPUSETMANAGER_H__
#define __CPUSETMANAGER_H__

#include <vector>
#include <sysutils/NetlinkEvent.h>

class CpusetManager {
private:
    static CpusetManager *sInstance;
    CpusetManager();

    bool isOnline(int num);
    int mMaxCpuNum;
    std::vector<int> mOnlineCpus;
    void setCpuset(const char *filename, int denominator);
    void updateCpuset();

public:
    virtual ~CpusetManager();
    int start();
    int stop();
    void handleCpuEvent(NetlinkEvent *event);

    static CpusetManager *Instance();
};

#endif
