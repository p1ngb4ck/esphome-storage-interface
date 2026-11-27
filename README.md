# esphome-storage-interface

## Purpose
One storage interface for ESPHome: SD cards, USB drives, network shares, flash chips and more register with one
registry, are reached by one path scheme (`/sdcard/...`, `/usb/...`, `/nfs/...`) and share one asynchronous worker
for copies, moves, listings and file streams. On top: file actions, exFAT, ISO images, a web file API and browser.

## How it works
`storage:` is the registry; it mounts nothing itself. Drivers (esphome-storage-drivers, esphome-usb-drivers) register
their storages under a mount path. One worker task, started only when a driver needs it, runs the jobs and streams
with a shared PSRAM transfer buffer; `storage.*` actions and conditions work on full paths. `web_server` (version 3,
ESP-IDF) adds the HTTP file API, a raw API and a file browser at `/files/`; `web_server_idf` carries the multipart
uploads. Take `storage` from this repository only.

## Quick start
Chips: ESP32, ESP32-S2, ESP32-S3, ESP32-P4

Needs: [esphome-esp32-platform](https://github.com/p1ngb4ck/esphome-esp32-platform),
[esphome-network](https://github.com/p1ngb4ck/esphome-network)

```yaml
external_components:
  - source: github://p1ngb4ck/esphome-storage-interface@main
    components: [storage, web_server, web_server_idf]
  - {source: github://p1ngb4ck/esphome-esp32-platform@main, components: [esp32]}
  - {source: github://p1ngb4ck/esphome-network@main, components: [network, ethernet]}

ethernet: {type: W5500, clk_pin: GPIO18, mosi_pin: GPIO21, miso_pin: GPIO14, cs_pin: GPIO13, interrupt_pin: GPIO4}
storage:
web_server: {version: 3, file_api: {max_dir_entries: 64}, file_browser: {variant: advanced}}
button:
  - platform: template
    name: Write log line
    on_press:
      - storage.file_append: {path: /sdcard/log.txt, content: "pressed"}
```

Use your board's pins (or your Wi-Fi). A driver registers a storage, e.g. `sd_storage` of
[esphome-storage-drivers](https://github.com/p1ngb4ck/esphome-storage-drivers) at `/sdcard`.

## Main options
| Option | What it does | Default |
|---|---|---|
| [`psram_transfer_buffer_size`](CONFIGURATION.md#storage) | shared PSRAM buffer for copies, streams | 25 % of PSRAM |
| [`max_pending` / `max_streams`](CONFIGURATION.md#storage) | jobs queued / file streams open at once | `4` / `2` |
| [`on_registered`](CONFIGURATION.md#storage) | automation when a storage registers | - |
| [`iso_mount`](CONFIGURATION.md#iso-images) | ISO images as read-only storages | - |
| [`storage.*` actions](CONFIGURATION.md#actions-and-conditions) | write, read, copy, move, list, mount, raw | - |
| [`web_server: file_api`](CONFIGURATION.md#file_api) | HTTP file endpoints under `/files/` | off |
| [`web_server: file_browser`](CONFIGURATION.md#file_browser) | file manager page, `simple` or `advanced` | `simple` |
| [`esp32: enable_exfat`](CONFIGURATION.md#esp32-exfat) | exFAT in FatFs | `false` |

## Catalog
- Components (owner here):
  - `storage`: [folder](https://github.com/p1ngb4ck/esphome-storage-interface/tree/main/esphome/components/storage),
    [configuration](CONFIGURATION.md#storage), [actions](CONFIGURATION.md#actions-and-conditions),
    [ISO images](CONFIGURATION.md#iso-images)
  - `web_server`:
    [folder](https://github.com/p1ngb4ck/esphome-storage-interface/tree/main/esphome/components/web_server),
    [configuration](CONFIGURATION.md#web_server-file-api-raw-api-file-browser)
  - `web_server_idf`:
    [folder](https://github.com/p1ngb4ck/esphome-storage-interface/tree/main/esphome/components/web_server_idf),
    [configuration](CONFIGURATION.md#web_server_idf)
  - `esp32` change (exFAT, FAT volumes; owner of `esp32` is esphome-esp32-platform):
    [folder](https://github.com/p1ngb4ck/esphome-storage-interface/tree/main/esphome/components/esp32),
    [configuration](CONFIGURATION.md#esp32-exfat)
- Branches:
  - [`main`](https://github.com/p1ngb4ck/esphome-storage-interface/tree/main): the whole topic
  - [`pr/esp32-exfat-fat-volumes`][b-exfat]
  - [`pr/storage`](https://github.com/p1ngb4ck/esphome-storage-interface/tree/pr/storage)
  - [`pr/storage-write-done`](https://github.com/p1ngb4ck/esphome-storage-interface/tree/pr/storage-write-done)
  - [`pr/web-server-idf-file-uploads`][b-uploads]
  - [`pr/web-server-file-api`](https://github.com/p1ngb4ck/esphome-storage-interface/tree/pr/web-server-file-api)
- Needed repos: [esphome-esp32-platform](https://github.com/p1ngb4ck/esphome-esp32-platform) (esp32),
  [esphome-network](https://github.com/p1ngb4ck/esphome-network) (network for the web server)
- Drivers: [esphome-storage-drivers](https://github.com/p1ngb4ck/esphome-storage-drivers),
  [esphome-usb-drivers](https://github.com/p1ngb4ck/esphome-usb-drivers)
- All topics together: `github://p1ngb4ck/esphome@dev`

[b-exfat]: https://github.com/p1ngb4ck/esphome-storage-interface/tree/pr/esp32-exfat-fat-volumes
[b-uploads]: https://github.com/p1ngb4ck/esphome-storage-interface/tree/pr/web-server-idf-file-uploads

---

# Detailed version

One storage interface for ESPHome: SD cards, USB drives, network shares, flash chips and more register with one
registry, are reached by one path scheme (`/sdcard/...`, `/usb/...`, `/nfs/...`) and share one asynchronous worker
for copies, moves, listings and file streams. On top of it: file actions for automations, exFAT, ISO images, and a
file API and file browser in the web server.

This repository holds the interface itself. The storage drivers are in
[esphome-storage-drivers](https://github.com/p1ngb4ck/esphome-storage-drivers) (SD card, NFS, FTP, S3, EEPROM /
FRAM / flash, ...) and [esphome-usb-drivers](https://github.com/p1ngb4ck/esphome-usb-drivers) (USB drives, optical
drives). Take `storage` from this repository only: other topic repos may still carry older copies of it.

`esp32` comes from its owner [esphome-esp32-platform](https://github.com/p1ngb4ck/esphome-esp32-platform)
(`LOOP_TASK_CORE`, `override_path`). The exFAT / FAT volume change of `esp32` (`pr/esp32-exfat-fat-volumes`,
`require_fatfs(volumes)`) is still a branch of this repository; FAT drivers (`sd_storage`, `usb_storage`) need it,
and esphome-esp32-platform's main does not have it yet.

## Examples

`storage:` alone mounts nothing; a driver registers its storage with it. With an SD card from
esphome-storage-drivers and the web server's file browser:

```yaml
external_components:
  - source: github://p1ngb4ck/esphome-storage-interface@main
    components: [storage, web_server, web_server_idf]
  - source: github://p1ngb4ck/esphome-esp32-platform@main
    components: [esp32]
  - source: github://p1ngb4ck/esphome-storage-drivers@main
    components: [sd_storage]

storage:

sd_storage:
  clk_pin: GPIO43
  cmd_pin: GPIO44
  data0_pin: GPIO39
  data1_pin: GPIO40
  data2_pin: GPIO41
  data3_pin: GPIO42

web_server:
  version: 3
  file_api:
    max_dir_entries: 64
  file_browser:
    variant: advanced
```

Use your board's pins. The file browser is then at `http://<device>/files/`. Write a file from an automation:

```yaml
button:
  - platform: template
    name: Write log line
    on_press:
      - storage.file_append:
          path: /sdcard/log.txt
          content: "pressed"
```

## More

- **[CONFIGURATION.md](CONFIGURATION.md)** - every option of `storage`, all `storage.*` actions with examples,
  exFAT, ISO images, the web server's file API, raw API and file browser.
- Every storage driver documents its own options in its repository's `CONFIGURATION.md`.

## Branches

One branch per change; `main` contains every branch. Base: ESPHome `dev` at `a5a75738ad`.

| Branch | Change | Chips | Needs |
|---|---|---|---|
| `pr/esp32-exfat-fat-volumes` | `esp32` exFAT, FAT volume count, VFS max count (1) | all ESP32, ESP-IDF | (2) |
| `pr/storage` | the `storage` component (3) | all ESP32 | (4) |
| `pr/storage-write-done` | `write_done()`: told when a written file is complete (5) | all ESP32 | `pr/storage` |
| `pr/web-server-idf-file-uploads` | `web_server_idf`: multipart uploads for the file API (6) | ESP-IDF | - |
| `pr/web-server-file-api` | `web_server`: `file_api`, `raw_api`, `file_browser` (7) | ESP-IDF, `version: 3` | (8) |

1. `pr/esp32-exfat-fat-volumes`: `esp32` `advanced: enable_exfat` (patched FatFs copy), FAT volume count from the
   configured mounts, VFS max count.
2. Needs `pr/esp32-idf-component-override-path`
   ([esphome-esp32-platform](https://github.com/p1ngb4ck/esphome-esp32-platform)).
3. `pr/storage`: registry, path / filesystem / network / raw storages, the asynchronous worker (jobs, streams),
   `storage.*` actions, PSRAM transfer buffer, change feed, ISO 9660 mount.
4. Needs `pr/esp32-exfat-fat-volumes`, `pr/esp32-loop-task-core`
   ([esphome-esp32-platform](https://github.com/p1ngb4ck/esphome-esp32-platform)).
5. Network storages finish uploads there.
6. Async uploads per handler, an explicit empty file created.
7. `file_browser`: `simple`, or `advanced` with js-fileexplorer.
8. Needs `pr/storage`, `pr/web-server-idf-file-uploads`.

All topics together: `github://p1ngb4ck/esphome@dev`. `external_components` replaces components only; the ESPHome
core of the build host stays as installed.

## License

- ESPHome's own files, also where this repository changes them: the [ESPHome License](LICENSE) (MIT for the Python
  code and everything else, GPLv3 for the C++ runtime files).
- New files written for this repository (new components, their code, tests and documentation): MIT,
  [LICENSE-MIT](LICENSE-MIT).
- Third-party code keeps its own licence:
  - `web_server/file_explorer/`: CubicleSoft js-fileexplorer, MIT (its `LICENSE.txt`).

## Thanks

The advanced file browser is CubicleSoft's [js-fileexplorer](https://github.com/cubiclesoft/js-fileexplorer) (MIT) -
thank you for such a complete widget. Everyone this project builds on:
[ATTRIBUTIONS.md](https://github.com/p1ngb4ck/esphome/blob/dev/ATTRIBUTIONS.md).

## Branch structure

Each `pr/` branch holds one part of this topic on top of upstream and says in its `BRANCH.md` what it is based on.
`main` holds all of them.

| Branch | Based on |
|---|---|
| `pr/esp32-exfat-fat-volumes` | `pr/esp32-idf-component-override-path` of p1ngb4ck/esphome-esp32-platform |
| `pr/storage` | `pr/esp32-exfat-fat-volumes`, `pr/esp32-loop-task-core` of p1ngb4ck/esphome-esp32-platform |
| `pr/storage-write-done` | `pr/storage` |
| `pr/web-server-file-api` | `pr/storage`, `pr/web-server-idf-file-uploads` |
| `pr/web-server-idf-file-uploads` | upstream only |
