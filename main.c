#include <vitasdk.h>
#include <taihen.h>

#define TOUCH_HOOKS 4
static tai_hook_ref_t refs[TOUCH_HOOKS];
static SceUID hooks[TOUCH_HOOKS] = {-1, -1, -1, -1};
static volatile int rear_mode = 0;

int sceTouchRead2(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs);
int sceTouchPeek2(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs);

static void poll_mode_buttons(void) {
    SceCtrlData pad;
    int ret = sceCtrlPeekBufferPositiveExt(0, &pad, 1);
    if (ret < 1) return;
    if (pad.buttons & SCE_CTRL_VOLDOWN) rear_mode = 1;
    if (pad.buttons & SCE_CTRL_VOLUP) rear_mode = 0;
}

static void clear_touch(SceTouchData *pData, SceUInt32 nBufs) {
    if (!pData) return;
    for (SceUInt32 i = 0; i < nBufs; i++) pData[i].reportNum = 0;
}

static void copy_front_as_rear(SceTouchData *dst, const SceTouchData *src, SceUInt32 nBufs) {
    if (!dst || !src) return;
    for (SceUInt32 i = 0; i < nBufs; i++) {
        dst[i].timeStamp = src[i].timeStamp;
        dst[i].status = src[i].status;
        SceUInt32 count = src[i].reportNum;
        if (count > SCE_TOUCH_MAX_REPORT) count = SCE_TOUCH_MAX_REPORT;
        dst[i].reportNum = count;
        for (SceUInt32 j = 0; j < count; j++) {
            dst[i].report[j].id = src[i].report[j].id;
            dst[i].report[j].force = src[i].report[j].force;
            dst[i].report[j].x = src[i].report[j].x;
            dst[i].report[j].y = (SceInt16)(((SceUInt32)src[i].report[j].y * 889U) / 1087U);
            dst[i].report[j].info = src[i].report[j].info;
            for (SceUInt32 k = 0; k < sizeof(dst[i].report[j].reserved); k++)
                dst[i].report[j].reserved[k] = src[i].report[j].reserved[k];
        }
    }
}

static int touch_patched_common(int ref_index, SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
    poll_mode_buttons();

    if (port == SCE_TOUCH_PORT_BACK) {
        if (!rear_mode) {
            int ret = TAI_CONTINUE(int, refs[ref_index], port, pData, nBufs);
            if (ret >= 0) clear_touch(pData, nBufs);
            return ret;
        }
        SceTouchData front[nBufs];
        for (SceUInt32 i = 0; i < nBufs; i++) front[i].reportNum = 0;
        int ret = TAI_CONTINUE(int, refs[ref_index], SCE_TOUCH_PORT_FRONT, front, nBufs);
        if (ret >= 0) copy_front_as_rear(pData, front, nBufs);
        else clear_touch(pData, nBufs);
        return ret;
    }

    int ret = TAI_CONTINUE(int, refs[ref_index], port, pData, nBufs);
    if (ret >= 0 && rear_mode && port == SCE_TOUCH_PORT_FRONT) clear_touch(pData, nBufs);
    return ret;
}

static int sceTouchRead_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) { return touch_patched_common(0, port, pData, nBufs); }
static int sceTouchRead2_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) { return touch_patched_common(1, port, pData, nBufs); }
static int sceTouchPeek_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) { return touch_patched_common(2, port, pData, nBufs); }
static int sceTouchPeek2_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) { return touch_patched_common(3, port, pData, nBufs); }

void _start() __attribute__((weak, alias("module_start")));

int module_start(SceSize argc, const void *args) {
    hooks[0] = taiHookFunctionImport(&refs[0], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0x169A1D58, sceTouchRead_patched);
    hooks[1] = taiHookFunctionImport(&refs[1], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0x39401BEA, sceTouchRead2_patched);
    hooks[2] = taiHookFunctionImport(&refs[2], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0xFF082DF0, sceTouchPeek_patched);
    hooks[3] = taiHookFunctionImport(&refs[3], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0x3AD3D0A1, sceTouchPeek2_patched);
    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
    for (int i = 0; i < TOUCH_HOOKS; i++) if (hooks[i] >= 0) taiHookRelease(hooks[i], refs[i]);
    return SCE_KERNEL_STOP_SUCCESS;
}
