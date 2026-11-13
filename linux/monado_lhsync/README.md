# monado_lhsync skeleton

Experimental Linux-side service for feeding QuestLHSync camera observations into a Monado / WiVRn session.

## Status
Skeleton only. Protocol connection and logging stubs; solver and Monado offset application are placeholders.

## Build (planned)
```bash
# Requires a C++17 compiler, and eventually Monado headers if embedding
mkdir build && cd build
cmake ..
make
```

## Run (planned)
```bash
./monado_lhsync --host <headset-ip>   # or auto-discover
```

It should:
- Connect to the existing Quest Magisk / Frame lhsyncd
- Log received frames / spots
- Eventually compute and apply the alignment offset to the Lighthouse tracking origin

See `../../docs/MONADO.md` for the overall design.
