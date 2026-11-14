# `pr/storage`

One part of the topic repo p1ngb4ck/esphome-storage-interface. The whole topic, with its description, is the branch `main`.

Change of this branch: the `storage` component: registry, path / filesystem / network / raw storages, the asynchronous worker (jobs, streams), `storage.*` actions, PSRAM transfer buffer, change feed, ISO 9660 mount

Based on these branches, whose changes it holds and builds on:

- `pr/esp32-exfat-fat-volumes`: `esp32` `advanced: enable_exfat` (patched FatFs copy), FAT volume count from the configured mounts, VFS max count
- `pr/esp32-loop-task-core` of p1ngb4ck/esphome-esp32-platform: `LOOP_TASK_CORE` in `esp32/hal.h`, the loop task pinned to it
