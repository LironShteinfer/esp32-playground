# ESP32-S3 Playground

Small ESP-IDF programs I wrote while learning the ESP32-S3's Wi-Fi and Bluetooth Low Energy radios. Each folder is a separate, self-contained ESP-IDF project.

| Project | What it does |
|---|---|
| [`wifi_scan`](wifi_scan) | Scans once for nearby Wi-Fi networks (including hidden ones) and prints SSID, signal strength (RSSI), channel and BSSID for each one. |
| [`wifi_connect`](wifi_connect) | Connects to a Wi-Fi network as a station, retries up to 5 times, prints the IP address it gets and then reports its status every 5 seconds. |
| [`wifi_sniff`](wifi_sniff) | Puts the radio in promiscuous mode on one channel and prints every captured frame: type (management / data / control), length, RSSI, channel and the transmitter's MAC address. |
| [`ble_scan`](ble_scan) | Runs a continuous BLE 5 extended scan (Bluedroid) and prints the MAC address, RSSI and address type of every advertising device. |

## Requirements

- **Board:** ESP32-S3 (all projects are configured for target `esp32s3`).
- **ESP-IDF 6.1** (the `sdkconfig` files were generated with it). Other recent versions should work, but may update `sdkconfig` on the first build.

## Build and flash

Each project is built on its own, from its folder:

```bash
cd wifi_scan
idf.py build
idf.py -p <PORT> flash monitor      # e.g. COM5 on Windows, /dev/ttyUSB0 on Linux
```

Exit the monitor with `Ctrl+]`.

### `wifi_connect`: set your network first

The network name and password are kept out of git:

```bash
cd wifi_connect/main
cp wifi_credentials.example.h wifi_credentials.h    # Windows: copy ... ...
```

Then edit `wifi_credentials.h` and set `WIFI_SSID` and `WIFI_PASS`. The file is listed in `.gitignore`, so it is never committed.

### `wifi_sniff`: choose the channel

The channel is set by `SNIFF_CHANNEL` at the top of `wifi_sniff/main/main.c` (default: 6). Run `wifi_scan` first to see which channel your network uses. Only sniff networks you own or have permission to monitor.

### `ble_scan`: Bluetooth configuration

Bluetooth (Bluedroid, BLE 5.0 features with extended scanning) is enabled in `ble_scan/sdkconfig`. If you regenerate the configuration, enable these again in `idf.py menuconfig` → Component config → Bluetooth.

## Notes

- Each project keeps its full `sdkconfig` in git so it builds with the same settings it was tested with.
- In `wifi_sniff`, control frames such as ACK and CTS do not carry a transmitter address, so the MAC printed for those frames is not meaningful.
