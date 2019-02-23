/**
 *
 */


#include <stdbool.h>
#include <stdio.h>
#include <windows.h>
#include <commctrl.h>

#include <jni.h>

#include "ghook_GlobalHook.h"



#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC ((USHORT) 0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE ((USHORT) 0x02)
#endif
#ifndef HID_USAGE_GENERIC_KEYBOARD
#define HID_USAGE_GENERIC_KEYBOARD ((USHORT) 0x06)
#endif

#define MOUSE_DOWN_FLAGS (RI_MOUSE_BUTTON_1_DOWN | RI_MOUSE_BUTTON_2_DOWN | RI_MOUSE_BUTTON_3_DOWN | RI_MOUSE_BUTTON_4_DOWN | RI_MOUSE_BUTTON_5_DOWN)
#define MOUSE_UP_FLAGS (RI_MOUSE_BUTTON_1_UP | RI_MOUSE_BUTTON_2_UP | RI_MOUSE_BUTTON_3_UP | RI_MOUSE_BUTTON_4_UP | RI_MOUSE_BUTTON_5_UP)

/** PROBABLY BAD:
    Right now, depending on property of WINAPI constants, (mouse_up * 2 == mouse_down)
    Should change logic so I don't depend on this quirk
    Or not, if it works it works
*/

USHORT vkeyHook = 0;

USHORT mouseDownHook = 0;
USHORT mouseUpHook = 0;

JavaVM *jvm = NULL;
jobject hookObj = NULL;
BOOL keyDown = FALSE;
jmethodID pressMeth = NULL;
jmethodID releaseMeth = NULL;
jmethodID changeKeyHook = NULL;
jmethodID changeMouseHook = NULL;

DWORD hookThreadId = 0;

/** TODO: monitor performance of attach / detach
    If they are only called when the PTT keys are pressed and released, shouldnt be too bad
*/



/*  */
LRESULT CALLBACK ghookHookEnabledProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    UINT dwSize;

    switch(uMsg) {
        case WM_INPUT:
            //wParam is either RIM_INPUT (this app foreground) or RIM_INPUTSINK (this app background)
            //lParam is the RAWINPUT handle
            //determine size of buffer
            if (GetRawInputData((HRAWINPUT) lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER)) == -1) {
                break;
            }

            LPBYTE lpb = (LPBYTE)malloc(dwSize);
            if (lpb == NULL) {
                break;
            }
            ZeroMemory(lpb, dwSize);

            //get actual data
            if (GetRawInputData((HRAWINPUT) lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
                free(lpb);
                break;
            }

            PRAWINPUT pRaw = (PRAWINPUT) lpb;
            JNIEnv* env;
            if (pRaw->header.dwType == RIM_TYPEKEYBOARD) {
                if (pRaw->data.keyboard.Flags == RI_KEY_MAKE && !keyDown && pRaw->data.keyboard.VKey == vkeyHook) {
                    keyDown = TRUE;
                    if ((*jvm)->AttachCurrentThread(jvm, (void**)&env, NULL) >= JNI_OK) {
                        (*env)->CallVoidMethod(env, hookObj, pressMeth);
                        (*jvm)->DetachCurrentThread(jvm);
                    }
                } else if (pRaw->data.keyboard.Flags == RI_KEY_BREAK && pRaw->data.keyboard.VKey == vkeyHook) {
                    keyDown = FALSE;
                    if ((*jvm)->AttachCurrentThread(jvm, (void**)&env, NULL) >= JNI_OK) {
                        (*env)->CallVoidMethod(env, hookObj, releaseMeth);
                        (*jvm)->DetachCurrentThread(jvm);
                    }
                }
            } else if (pRaw->header.dwType == RIM_TYPEMOUSE) {
                if (pRaw->data.mouse.usButtonFlags & mouseDownHook) {
                    if ((*jvm)->AttachCurrentThread(jvm, (void**)&env, NULL) >= JNI_OK) {
                        (*env)->CallVoidMethod(env, hookObj, pressMeth);
                        (*jvm)->DetachCurrentThread(jvm);
                    }
                } else if (pRaw->data.mouse.usButtonFlags & mouseUpHook) {
                    if ((*jvm)->AttachCurrentThread(jvm, (void**)&env, NULL) >= JNI_OK) {
                        (*env)->CallVoidMethod(env, hookObj, releaseMeth);
                        (*jvm)->DetachCurrentThread(jvm);
                    }
                }
            }
            free(lpb);
            break;
        case WM_NCDESTROY:
            //printf("Removing Windows Subclass!\n");
            RemoveWindowSubclass(hWnd, &ghookHookEnabledProc, 1);
            break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}


/*  */
LRESULT CALLBACK ghookChooseKeyCodeProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    UINT dwSize;
    switch(uMsg) {
        case WM_INPUT:
            if (GetRawInputData((HRAWINPUT) lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER)) == -1) {
                break;
            }
            LPBYTE lpb = (LPBYTE)malloc(dwSize);
            if (lpb == NULL) {
                break;
            }
            ZeroMemory(lpb, dwSize);
            if (GetRawInputData((HRAWINPUT) lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
                free(lpb);
                break;
            }

            PRAWINPUT pRaw = (PRAWINPUT) lpb;
            JNIEnv* env;
            if (pRaw->header.dwType == RIM_TYPEKEYBOARD) {
                if (pRaw->data.keyboard.Flags == RI_KEY_MAKE) {
                    if ((*jvm)->AttachCurrentThread(jvm, (void**)&env, NULL) >= JNI_OK) {
                        vkeyHook = pRaw->data.keyboard.VKey;
                        (*env)->CallVoidMethod(env, hookObj, changeKeyHook, pRaw->data.keyboard.VKey);
                        (*jvm)->DetachCurrentThread(jvm);
                        RemoveWindowSubclass(hWnd, &ghookChooseKeyCodeProc, 1);
                        SetWindowSubclass(hWnd, &ghookHookEnabledProc, 1, 0);
                    }
                }
            } else if (pRaw->header.dwType == RIM_TYPEMOUSE) {
                if (pRaw->data.mouse.usButtonFlags & MOUSE_DOWN_FLAGS) {
                    if ((*jvm)->AttachCurrentThread(jvm, (void**)&env, NULL) >= JNI_OK) {
                        mouseDownHook = pRaw->data.mouse.usButtonFlags;
                        mouseUpHook = 2 * pRaw->data.mouse.usButtonFlags;
                        (*env)->CallVoidMethod(env, hookObj, changeMouseHook, pRaw->data.mouse.usButtonFlags);
                        (*jvm)->DetachCurrentThread(jvm);
                        RemoveWindowSubclass(hWnd, &ghookChooseKeyCodeProc, 1);
                        SetWindowSubclass(hWnd, &ghookHookEnabledProc, 1, 0);
                    }
                }
            }
            free(lpb);
            break;
        case WM_NCDESTROY:
            //printf("Removing Windows Subclass!\n");
            RemoveWindowSubclass(hWnd, &ghookChooseKeyCodeProc, 1);
            break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}


/*  */
BOOL register_ghook(HWND hWnd) {
    INT nResult;
    PRAWINPUTDEVICE rawInputDevices = malloc(2 * sizeof(RAWINPUTDEVICE));
    ZeroMemory(rawInputDevices, 2 * sizeof(*rawInputDevices));

    rawInputDevices[0].dwFlags = RIDEV_INPUTSINK;
    rawInputDevices[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rawInputDevices[0].usUsage = HID_USAGE_GENERIC_KEYBOARD;
    rawInputDevices[0].hwndTarget = hWnd;

    rawInputDevices[1].dwFlags = RIDEV_INPUTSINK;
    rawInputDevices[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rawInputDevices[1].usUsage = HID_USAGE_GENERIC_MOUSE;
    rawInputDevices[1].hwndTarget = hWnd;

    nResult = RegisterRawInputDevices(rawInputDevices, 2, sizeof(*rawInputDevices));

    if (nResult == TRUE) {
        //printf("Setting windows subclass\n");
        return SetWindowSubclass(hWnd, &ghookChooseKeyCodeProc, 1, 0);
    } else {
      //printf("register raw input devices failed %ld\n", GetLastError());
      return FALSE;
    }
}




JNIEXPORT jshort JNICALL Java_ghook_GlobalHook_getInputKeyCode
  (JNIEnv *env, jobject obj) {
    return 1;
}


JNIEXPORT jboolean JNICALL Java_ghook_GlobalHook_registerKeyboardHook
  (JNIEnv *env, jobject obj) {
    if(jvm==NULL) {
        (*env)->GetJavaVM(env, &jvm);
    }
    hookThreadId = GetCurrentThreadId();

    hookObj = (*env)->NewGlobalRef(env, obj);
    jclass hookCls = (*env)->GetObjectClass(env, hookObj);
    pressMeth = (*env)->GetMethodID(env, hookCls, "pressFunc", "()V");
    releaseMeth = (*env)->GetMethodID(env, hookCls, "releaseFunc", "()V");
    changeKeyHook = (*env)->GetMethodID(env, hookCls, "changeKeyHook", "(S)V");
    changeMouseHook = (*env)->GetMethodID(env, hookCls, "changeMouseHook", "(S)V");

    if (pressMeth == 0 || releaseMeth == 0) {
        (*env)->ExceptionClear(env);
        //printf("NATIVE: registerHook - No handle method!\n");
        return 0;
    }


    HWND window_tgt = GetActiveWindow();
    return register_ghook(window_tgt);
}

JNIEXPORT jboolean JNICALL Java_ghook_GlobalHook_deregisterKeyboardHook
  (JNIEnv *env, jobject obj) {
    HWND window_tgt = GetActiveWindow();
    vkeyHook = 0;
    mouseDownHook = 0;
    mouseUpHook = 0;
    (*env)->CallVoidMethod(env, hookObj, changeKeyHook, 0);
    (*env)->CallVoidMethod(env, hookObj, changeKeyHook, 0);
    (*env)->CallVoidMethod(env, hookObj, changeKeyHook, 0);
    BOOL x = RemoveWindowSubclass(window_tgt, &ghookChooseKeyCodeProc, 1);
    BOOL y = RemoveWindowSubclass(window_tgt, &ghookHookEnabledProc, 1);
    return x || y;
}
