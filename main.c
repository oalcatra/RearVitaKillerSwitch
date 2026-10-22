#include <vitasdk.h>
#include <taihen.h>

static tai_hook_ref_t refs[4];
static SceUID hooks[4] = {-1, -1, -1, -1};

int sceTouchRead2(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs);
int sceTouchPeek2(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs);

static void kill_rear(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
    if (port != SCE_TOUCH_PORT_BACK || !pData) return;
    for (SceUInt32 i = 0; i < nBufs; i++) {
        pData[i].reportNum = 0;
    }
}

static int sceTouchRead_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
    int ret = TAI_CONTINUE(int, refs[0], port, pData, nBufs);
    if (ret >= 0) kill_rear(port, pData, nBufs);
    return ret;
}

static int sceTouchRead2_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
    int ret = TAI_CONTINUE(int, refs[1], port, pData, nBufs);
    if (ret >= 0) kill_rear(port, pData, nBufs);
    return ret;
}

static int sceTouchPeek_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
    int ret = TAI_CONTINUE(int, refs[2], port, pData, nBufs);
    if (ret >= 0) kill_rear(port, pData, nBufs);
    return ret;
}

static int sceTouchPeek2_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
    int ret = TAI_CONTINUE(int, refs[3], port, pData, nBufs);
    if (ret >= 0) kill_rear(port, pData, nBufs);
    return ret;
}

void _start() __attribute__((weak, alias("module_start")));

int module_start(SceSize argc, const void *args) {
    hooks[0] = taiHookFunctionImport(&refs[0], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0x169A1D58, sceTouchRead_patched);
    hooks[1] = taiHookFunctionImport(&refs[1], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0x39401BEA, sceTouchRead2_patched);
    hooks[2] = taiHookFunctionImport(&refs[2], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0xFF082DF0, sceTouchPeek_patched);
    hooks[3] = taiHookFunctionImport(&refs[3], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0x3AD3D0A1, sceTouchPeek2_patched);
    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
    for (int i = 0; i < 4; i++) {
        if (hooks[i] >= 0) taiHookRelease(hooks[i], refs[i]);
    }
    return SCE_KERNEL_STOP_SUCCESS;
}
