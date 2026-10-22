#include <vitasdkkern.h>
#include <taihen.h>
#include <psp2common/ctrl.h>

#define KERNEL_PID 0x10005

static tai_hook_ref_t ctrl_ref;
static SceUID ctrl_hook = -1;
static volatile int rear_mode = 0;

/* User plugin reads this through the syscall export. */
int rearVitaGetMode(void) {
    return rear_mode;
}

/* SceCtrl internal Positive2 is the same path reVita uses for the extra/system
 * buttons. Hooking it globally lets us observe SceShell's own reads, where the
 * physical volume keys are visible even though games do not receive them.
 */
static int ksceCtrlPeekBufferPositive2_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, ctrl_ref, port, pad_data, count);

    if (ret > 0 && pad_data) {
        for (int i = 0; i < ret; i++) {
            if (pad_data[i].buttons & SCE_CTRL_VOLDOWN)
                rear_mode = 1;
            if (pad_data[i].buttons & SCE_CTRL_VOLUP)
                rear_mode = 0;
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

    /* 3.60/3.65 SceCtrl internal ksceCtrlPeekBufferPositive2 offset used by reVita. */
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
