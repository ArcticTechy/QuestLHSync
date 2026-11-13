# QuestLHSync for Monado / WiVRn

## Goal
Adapt the continuous camera-based Lighthouse alignment of QuestLHSync to a Linux Monado (and thus WiVRn) runtime.

The headset-side component (Quest Magisk module or Steam Frame service) stays unchanged: it still serves bright-spot observations of base-station laser flashes over the local network.

## Alignment model (from original)
The solver (primarily in `src/driver/sync.cpp` / `sync.h`) computes a 4-DOF transform (yaw + 3D translation) between:
- The headset's own STAGE space (gravity-aligned), and
- A reference Lighthouse frame (derived from base-station poses, optionally leveled).

Form: `p_headset = R_yaw * p_lighthouse_ref + t`

This is applied continuously, with slew limiting, gravity leveling, station identity checks, etc.

## Monado integration approach

Monado's `steamvr_lh` driver wraps the proprietary SteamVR Lighthouse driver and exposes Lighthouse devices under their own `xrt_tracking_origin` (the `Context` class inherits from it).

WiVRn's streamed HMD has a separate tracking origin.

**Primary strategy**: Apply the computed transform as a runtime offset on the Lighthouse tracking origin using the space overseer:

```c
xrt_space_overseer_set_tracking_origin_offset(xso, lh_origin, &computed_offset);
```

(See `xrt_space_overseer` in Monado: `set_tracking_origin_offset` / `get_tracking_origin_offset`.)

This keeps all Lighthouse devices (controllers, trackers, base stations) consistently transformed relative to the WiVRn HMD without modifying individual device poses.

### Alternative / complementary
- Inject as a custom driver that provides corrected poses.
- Use motoc-style one-time calibration as a fallback or seed.
- Environment variables like `OXR_TRACKING_ORIGIN_OFFSET_*` are too coarse (global, not continuous).

## Skeleton service
A Linux-side daemon (see `linux/monado_lhsync/`) is intended to:
1. Discover and connect to the headset using the same protocol as `HeadsetLink` (UDP 47281 / TCP 47280).
2. Receive camera frames / spots and HMD pose history (the latter may need to come from Monado itself).
3. Run (or call into) the alignment solver.
4. Publish the current 4-DOF correction (e.g. via a control socket, shared memory, or by becoming a Monado component).

## Open challenges
- Obtaining the WiVRn/Monado HMD pose history at exposure time (the original uses SteamVR's raw poses).
- Accessing the space overseer / tracking origins from an external process (may require embedding the logic as a Monado driver or using an IPC that Monado exposes).
- Porting the large C++ solver (`sync.cpp` is ~158 kB) and its dependencies cleanly to Linux.
- Timing, clock sync, and gravity leveling adapted to Monado's clocks and spaces.
- Multi-headset and station.json reference frame handling.

## Next steps
1. Minimal Linux client that connects and logs spots (reuse or reimplement net protocol).
2. Extract or wrap the Solver for Linux.
3. Prototype applying a fixed offset via space overseer to validate the approach.
4. Integrate continuous updates.

The original Windows SteamVR path remains fully functional on the `master` branch.
