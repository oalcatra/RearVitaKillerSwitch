#include <vitasdkkern.h>
#include <taihen.h>
#include <psp2common/ctrl.h>

#define KERNEL_PID 0x10005

static tai_hook_ref_t ctrl_ref;
static SceUID ctrl_hook = -1;
static volatile int rear_mode = 0;

/* We use L3/R3 only as an internal bridge. A PCH-1000 has no physical L3/R3.
 * The user plugin consumes these markers and strips them before returning pad
 * data to the application.
 */
#define MODE_REAR_MARKER  SCE_CTRL_L3
#define MODE_FRONT_MARKER SCE_CTRL_R3

static int ksceCtrlPeekBufferPositive2_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, ctrl_ref, port, pad_data, count);

    if (ret > 0 && pad_data) {
        for (int i = 0; i < ret; i++) {
            if (pad_data[i].buttons & SCE_CTRL_VOLDOWN)
                rear_mode = 1;
            if (pad_data[i].buttons & SCE_CTRL_VOLUP)
                rear_mode = 0;

            if (rear_mode)
                pad_data[i].buttons |= MODE_REAR_MARKER;
            else
                pad_data[i].buttons |= MODE_FRONT_MARKER;
        }
    }

    return ret;
}

void _start() __attribute__((weak, alias("module_start")));

int module_start(SceSize argc, const void *args) {
    tai_module_info_t info;
    info.size = sizeof(info);

    if (taiGetModuleInfoForKernel(KERNEL_PID, "SceCtrl", &info) < 0)
        return SCE_KERNEL_START_FAILED;

    ctrl_hook = taiHookFunctionOffsetForKernel(
        KERNEL_PID, &ctrl_ref, info.modid, 0, 0x3EF8 | 1, 1,
        ksceCtrlPeekBufferPositive2_patched);

    if (ctrl_hook < 0)
        return SCE_KERNEL_START_FAILED;

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
    if (ctrl_hook >= 0)
        taiHookReleaseForKernel(ctrl_hook, ctrl_ref);
    return SCE_KERNEL_STOP_SUCCESS;
}
