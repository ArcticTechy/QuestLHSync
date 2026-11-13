# monado_lhsync — Linux / Monado control client

Terminal-based control panel + network client that speaks the QuestLHSync headset protocol.

## Build
```bash
mkdir build && cd build
cmake ..
make
```

## Run
```bash
./monado_lhsync                 # auto-discover on the LAN
./monado_lhsync --host 192.168.1.42
```

## Controls
- `p` — pause / resume corrections
- `r` — force recalibrate (stub)
- `q` — quit

The UI shows connection state, frame count, and a stub alignment transform.

## Status
- Network discovery + TCP session: implemented (POSIX).
- Frame / hello / calibration parsing: basic.
- Alignment solver: stub (slowly drifts yaw for demo).
- Monado `set_tracking_origin_offset`: not yet wired (see `docs/MONADO.md`).

Requires a rooted Quest (or Steam Frame) with the original Magisk / Frame module running so the headset answers discovery.
