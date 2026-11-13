# Monado / WiVRn integration notes

## Getting HMD poses

The Linux client needs the current HMD pose (in the same time base as the camera frames) so the alignment solver can compute the 4-DOF correction between the Lighthouse frame and the headset frame.

### Recommended path: Monado IPC client

Monado (and WiVRn) run a service process (`monado-service`). Clients connect via a Unix socket (typically `$XDG_RUNTIME_DIR/monado_comp_ipc` or the WiVRn equivalent) and shared memory.

Key APIs (from Monado's IPC client layer):

- `ipc_instance_create()` — connect and create an `xrt_instance`
- Device enumeration → find the HMD
- `xrt_device::get_tracked_pose(XRT_INPUT_GENERIC_HEAD_POSE, timestamp, &relation)`
- Space overseer: `xrt_space_overseer_set_tracking_origin_offset()` on the Lighthouse tracking origin

A skeleton is in `linux/monado_lhsync/monado_ipc.cpp`.

### Alternative (easier to prototype)

Write a tiny OpenXR client that:
1. Creates an instance + session against Monado/WiVRn
2. Locates the head space at high rate
3. Streams the poses over a local socket to the lhsync process

This avoids linking against Monado internals but adds an extra process.

## Applying the correction

Once the solver produces a yaw + translation offset, apply it with:

```c
xrt_space_overseer_set_tracking_origin_offset(xso, lighthouse_origin, &offset_pose);
```

This moves the entire Lighthouse tracking origin relative to the headset, which is exactly what the original SteamVR driver does.

## Next implementation steps

1. Decide between pure IPC client vs. OpenXR sidecar.
2. Port the core solver from the original `sync.cpp` (the 4-DOF alignment).
3. Wire the pose source into the solver and the resulting offset into the space overseer.
4. Keep the terminal UI for status / pause / force-recalibrate.
