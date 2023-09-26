# WM_POINTER Syscalls

There are a few syscalls that seem to implement most of the `WM_POINTER*`-related function calls in User32. Thankfully, they mostly seem to be self-explanatory and thus it should be possible to deduce most of what is going on without needing to resort to disassembling any code.

When syscalls appear to have the same arguments, return value, and error handling as the User32 function, they are called out as "equivalent".

> **TODO**: Research differences between native x86/native x64/WOW64

## Event promotion

When a `WM_POINTER*` event reaches `user32!DefWindowProc`, it will be "promoted" to `WM_MOUSE*` and a new `WM_MOUSE*` event will be added to the queue that corresponds to the `WM_POINTER*` event.

There are two syscalls that seem to be related here, but for "native" pointer events, e.g. ones coming from a pen or touch sensor, `win32u!NtUserPromotePointer` will promote a pointer event that is currently being handled into `WM_MOUSE*`.

* `win32u!NtUserPromotePointer`: Two arguments, unknown but significant values.

Note that for promotion to work, the events must have actually been sent by Windows; there is no way for a fake event to go through this promotion process unless it is sent via the synthetic pointer API.

## Mouse-in-Pointer functionality

The `WM_POINTER*` events are automatically translated into `WM_MOUSE*` events by `DefWindowProc`, so any "proper" window procedure should work just fine with `WM_POINTER*` events even if they are not aware of them.

However, nothing is actually stopping poorly-behaved applications from still doing the wrong thing when `WM_POINTER*` events should be passed to `user32!DefWindowProc`, so Windows defaults to still sending `WM_MOUSE*` events to all applications.

In order to enable or disable this functionality explicitly, developers can call `user32!EnableMouseInPointer`. Once called, it is stated that the setting is permanent for the lifetime of the process.

The syscall interface appears to have two relevant procedures:

* `win32u!NtUserEnableMouseInPointer`: Equivalent to `user32!EnableMouseInPointer`
* `win32u!NtUserEnableMouseInPointerForWindow`: Appears to be per-HWND instead of per-process, but not documented. Two arguments.

> **TODO**: Investigate	`win32u!NtUserEnableMouseInPointerForWindow` behavior/arguments.

When enabled, `WM_POINTER*` events still get promoted to `WM_MOUSE*` if they are passed to `DefWindowProc`. From the perspective of normal usage, it seems indistinguishable from any other `WM_POINTER*` event.

Internally, however, to promote a `WM_POINTER*` even that was originally a mouse event, you must call `win32u!NtUserPromoteMouseInPointer` instead of `win32u!NtUserPromotePointer`.

* `win32u!NtUserPromoteMouseInPointer`: Single argument; unknown significance. Passing zero seems to work.

Like `win32u!NtUserPromotePointer`, this function does not appear to work with fake events; Any events not created by Windows as mouse-in-pointer events will silently not be promoted.

## Data retrieval

* `win32u!NtUserGetPointerCursorId`: Equivalent to `user32!GetPointerCursorId`.
* `win32u!NtUserGetPointerDevice`: Equivalent to `user32!GetPointerDevice`.
* `win32u!NtUserGetPointerDeviceCursors`: Equivalent to `user32!GetPointerDeviceCursors`.
* `win32u!NtUserGetPointerDeviceInputSpace`: Unknown.
* `win32u!NtUserGetPointerDeviceOrientation`: Unknown.
* `win32u!NtUserGetPointerDeviceProperties`: Unknown.
* `win32u!NtUserGetPointerDeviceRects`: Unknown.
* `win32u!NtUserGetPointerDevices`: Equivalent to `user32!GetPointerDevices`.
* `win32u!NtUserGetPointerFrameArrivalTimes`: Unknown.
* `win32u!NtUserGetPointerFrameTimes`: Unknown.
* `win32u!NtUserGetPointerInfoList`: Unknown. Possibly related to list functions such as `user32!GetPointerFrameInfoHistory`.
* `win32u!NtUserGetPointerInputTransform`: Equivalent to `user32!GetPointerInputTransform`.
* `win32u!NtUserGetPointerProprietaryId`: Unknown.
* `win32u!NtUserGetPointerType`: Equivalent to `user32!GetPointerType`.

More research is needed, as it is unclear how most of the data is retrieved using the syscall interface right now.

## Synthetic Pointers

Fairly straight-forward syscall interface. An example of this can be seen in `MainWindow::InjectEvents`.

* `win32u!NtUserInitializePointerDeviceInjection`: Similar to `user32!CreateSyntheticPointerDevice`. However, it has some different arguments. Instead of returning `HSYNTHETICPOINTERDEVICE`, the last argument is a pointer to an `HSYNTHETICPOINTERDEVICE` handle that it will write to. Also, there is a third parameter whose significance is unknown; passing zero seems to work fine.
* `win32u!NtUserInjectPointerInput`: Equivalent to `user32!InjectSyntheticPointerInput`.
* `win32u!NtUserRemoveInjectionDevice`: Equivalent to `user32!DestroySyntheticPointerDevice`.
