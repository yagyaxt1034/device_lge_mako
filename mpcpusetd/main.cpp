// vim: ts=4 sw=4 expandtab

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <cutils/sockets.h>

#include "CpusetManager.h"
#include "NetlinkManager.h"

int main(int argc, char** argv) {
    setenv("ANDROID_LOG_TAGS", "*:v", 1);
    android::base::InitLogging(argv, android::base::LogdLogger(android::base::SYSTEM));
    LOG(INFO) << "mpcpusetd 0.1 fireing up";

    CpusetManager *cm;
    NetlinkManager *nm;

    if (!(cm = CpusetManager::Instance())) {
        LOG(ERROR) << "Unable to create CpusetManager";
        exit(1);
    }

    if (!(nm = NetlinkManager::Instance())) {
        LOG(ERROR) << "Unable to create NetlinkManager";
        exit(1);
    }

    if (cm->start()) {
        PLOG(ERROR) << "Unable to start CpusetManager";
        exit(1);
    }
    if (nm->start()) {
        PLOG(ERROR) << "Unable to start NetlinkHandler";
        exit(1);
    }

    while(1) {
        sleep(1000);
    }

    LOG(ERROR) << "mcpusetd exiting";
    exit(0);
}

