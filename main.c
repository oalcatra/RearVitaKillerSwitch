#include <vitasdk.h>
#include <taihen.h>

#define TOUCH_HOOKS 4
#define CTRL_HOOKS  4
#define TOTAL_HOOKS (TOUCH_HOOKS + CTRL_HOOKS)

static tai_hook_ref_t refs[TOTAL_HOOKS];
static SceUID hooks[TOTAL_HOOKS] = {-1, -1, -1, -1, -1, -1, -1, -1};

/* 0 = normal front touch, rear disabled
 * 1 = front panel is exposed to the app as rear touch, front disabled
 */
static volatile int rear_mode = 0;

int sceTouchRead2(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs);
int sceTouchPeek2(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs);

static void clear_touch(SceTouchData *pData, SceUInt32 nBufs) {
    if (!pData)
        return;

    for (SceUInt32 i = 0; i < nBufs; i++)
        pData[i].reportNum = 0;
}

static void copy_front_as_rear(SceTouchData *dst, const SceTouchData *src, SceUInt32 nBufs) {
    if (!dst || !src)
        return;

    for (SceUInt32 i = 0; i < nBufs; i++) {
        dst[i].timeStamp = src[i].timeStamp;
        dst[i].status = src[i].status;

        SceUInt32 count = src[i].reportNum;
        if (count > SCE_TOUCH_MAX_REPORT)
            count = SCE_TOUCH_MAX_REPORT;
        dst[i].reportNum = count;

        for (SceUInt32 j = 0; j < count; j++) {
            dst[i].report[j].id = src[i].report[j].id;
            dst[i].report[j].force = src[i].report[j].force;
            dst[i].report[j].x = src[i].report[j].x;
            dst[i].report[j].y =
                (SceInt16)(((SceUInt32)src[i].report[j].y * 889U) / 1087U);
            dst[i].report[j].info = src[i].report[j].info;

            for (SceUInt32 k = 0; k < sizeof(dst[i].report[j].reserved); k++)
                dst[i].report[j].reserved[k] = src[i].report[j].reserved[k];
        }
    }
}

/* Use TAI_CONTINUE for the FRONT read too. This bypasses our own hook and
 * avoids recursion while letting a rear request be replaced with front data.
 */
static int touch_patched_common(int ref_index, SceUInt32 port,
                                SceTouchData *pData, SceUInt32 nBufs) {
    if (port == SCE_TOUCH_PORT_BACK) {
        if (!rear_mode) {
            int ret = TAI_CONTINUE(int, refs[ref_index], port, pData, nBufs);
            if (ret >= 0)
                clear_touch(pData, nBufs);
            return ret;
        }

        SceTouchData front[nBufs];
        for (SceUInt32 i = 0; i < nBufs; i++)
            front[i].reportNum = 0;

        int ret = TAI_CONTINUE(int, refs[ref_index], SCE_TOUCH_PORT_FRONT, front, nBufs);
        if (ret >= 0)
            copy_front_as_rear(pData, front, nBufs);
        else
            clear_touch(pData, nBufs);
        return ret;
    }

    int ret = TAI_CONTINUE(int, refs[ref_index], port, pData, nBufs);

    /* In rear mode the physical front panel becomes rear-only for the app. */
    if (ret >= 0 && rear_mode && port == SCE_TOUCH_PORT_FRONT)
        clear_touch(pData, nBufs);

    return ret;
}

static int sceTouchRead_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
    return touch_patched_common(0, port, pData, nBufs);
}

static int sceTouchRead2_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
    return touch_patched_common(1, port, pData, nBufs);
}

static int sceTouchPeek_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
    return touch_patched_common(2, port, pData, nBufs);
}

static int sceTouchPeek2_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
    return touch_patched_common(3, port, pData, nBufs);
}

static void handle_mode_buttons(SceCtrlData *pad_data, int count) {
    if (!pad_data || count <= 0)
        return;

    for (int i = 0; i < count; i++) {
        if (pad_data[i].buttons & SCE_CTRL_VOLDOWN)
            rear_mode = 1;
        if (pad_data[i].buttons & SCE_CTRL_VOLUP)
            rear_mode = 0;
    }
}

static int sceCtrlPeekBufferPositive_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, refs[4], port, pad_data, count);
    if (ret > 0)
        handle_mode_buttons(pad_data, ret);
    return ret;
}

static int sceCtrlReadBufferPositive_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, refs[5], port, pad_data, count);
    if (ret > 0)
        handle_mode_buttons(pad_data, ret);
    return ret;
}

static int sceCtrlPeekBufferPositive2_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, refs[6], port, pad_data, count);
    if (ret > 0)
        handle_mode_buttons(pad_data, ret);
    return ret;
}

static int sceCtrlReadBufferPositive2_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, refs[7], port, pad_data, count);
    if (ret > 0)
        handle_mode_buttons(pad_data, ret);
    return ret;
}

void _start() __attribute__((weak, alias("module_start")));

int module_start(SceSize argc, const void *args) {
    /* Touch API */
    hooks[0] = taiHookFunctionImport(&refs[0], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0x169A1D58, sceTouchRead_patched);
    hooks[1] = taiHookFunctionImport(&refs[1], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0x39401BEA, sceTouchRead2_patched);
    hooks[2] = taiHookFunctionImport(&refs[2], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0xFF082DF0, sceTouchPeek_patched);
    hooks[3] = taiHookFunctionImport(&refs[3], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0x3AD3D0A1, sceTouchPeek2_patched);

    /* Controller API. VOL- selects rear mode; VOL+ selects front mode. */
    hooks[4] = taiHookFunctionImport(&refs[4], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0xA9C3CED6, sceCtrlPeekBufferPositive_patched);
    hooks[5] = taiHookFunctionImport(&refs[5], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0x67E7AB83, sceCtrlReadBufferPositive_patched);
    hooks[6] = taiHookFunctionImport(&refs[6], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0x15F81E8C, sceCtrlPeekBufferPositive2_patched);
    hooks[7] = taiHookFunctionImport(&refs[7], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0xC4226A3E, sceCtrlReadBufferPositive2_patched);

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
    for (int i = 0; i < TOTAL_HOOKS; i++) {
        if (hooks[i] >= 0)
            taiHookRelease(hooks[i], refs[i]);
    }
    return SCE_KERNEL_STOP_SUCCESS;
}
