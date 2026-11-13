# Simplified solver notes

The full original `Solver` in `src/driver/sync.cpp` is several thousand lines of carefully tuned logic (frame-grid timing, clock sync, outlier rejection, layout validation, height locking, etc.).

For the Monado port we start with a **minimal geometric 4-DOF fitter** (`simple_solver.cpp`):

- Takes a set of observations: (ray origin + direction in headset frame, corresponding station position in lighthouse frame)
- Searches yaw, then least-squares translation
- Returns residual and inlier count

This is enough to produce a usable correction once we have:
1. Rays from the headset cameras (already parsed from the network protocol)
2. Station poses (from `stations.json` or Monado’s steamvr_lh driver)
3. HMD poses at the correct timestamps (from Monado IPC)

Later we can progressively port more of the original timing and robustness logic.
