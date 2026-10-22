#include <vitasdk.h>
#include <taihen.h>

static tai_hook_ref_t refs[4];
static SceUID hooks[4] = {-1, -1, -1, -1};

int sceTouchRead2(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs);
int sceTouchPeek2(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs);

/*
 * Phase 2 test:
 * - The real rear panel is never exposed to the application.
 * - Whenever the app asks for rear touch, read the FRONT panel instead.
 * - Copy the front reports into the rear result and scale coordinates from
 *   the front active range to the rear active range.
 *
 * Vita touch coordinate ranges:
 *   front: X 0..1919, Y 0..1087
 *   rear : X 0..1919, Y 0..889
 */
static void front_to_rear(SceTouchData *pData, SceUInt32 nBufs, int use_peek) {
    if (!pData || nBufs == 0)
        return;

    SceTouchData front[nBufs];

    /* Avoid libc: initialize only what is needed before the touch call. */
    for (SceUInt32 i = 0; i < nBufs; i++)
        front[i].reportNum = 0;

    int ret;
    if (use_peek)
        ret = sceTouchPeek(SCE_TOUCH_PORT_FRONT, front, nBufs);
    else
        ret = sceTouchRead(SCE_TOUCH_PORT_FRONT, front, nBufs);

    if (ret < 0) {
        for (SceUInt32 i = 0; i < nBufs; i++)
            pData[i].reportNum = 0;
        return;
    }

    for (SceUInt32 i = 0; i < nBufs; i++) {
        /* Struct assignment lets GCC copy the data without calling memcpy. */
        pData[i] = front[i];

        for (SceUInt32 j = 0; j < pData[i].reportNum; j++) {
            /* X ranges are the same. Rear Y is shorter than front Y. */
            pData[i].report[j].y =
                (SceUInt16)(((SceUInt32)pData[i].report[j].y * 889U) / 1087U);
        }
    }
}

static int sceTouchRead_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
    int ret = TAI_CONTINUE(int, refs[0], port, pData, nBufs);

    if (ret >= 0 && port == SCE_TOUCH_PORT_BACK)
        front_to_rear(pData, nBufs, 0);

    return ret;
}

static int sceTouchRead2_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
    int ret = TAI_CONTINUE(int, refs[1], port, pData, nBufs);

    if (ret >= 0 && port == SCE_TOUCH_PORT_BACK)
        front_to_rear(pData, nBufs, 0);

    return ret;
}

static int sceTouchPeek_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
    int ret = TAI_CONTINUE(int, refs[2], port, pData, nBufs);

    if (ret >= 0 && port == SCE_TOUCH_PORT_BACK)
        front_to_rear(pData, nBufs, 1);

    return ret;
}

static int sceTouchPeek2_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
    int ret = TAI_CONTINUE(int, refs[3], port, pData, nBufs);

    if (ret >= 0 && port == SCE_TOUCH_PORT_BACK)
        front_to_rear(pData, nBufs, 1);

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
        if (hooks[i] >= 0)
            taiHookRelease(hooks[i], refs[i]);
    }
    return SCE_KERNEL_STOP_SUCCESS;
}
