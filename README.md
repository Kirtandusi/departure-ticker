# Departure ticker

Madison Metro departure times for a Raspberry Pi 5 and a 64×32 RGB LED panel.
The app shows the next usable departure at each configured stop. It also runs
in a terminal without a panel.

## Run in a container

Docker must be running. Build and check the console app:

```sh
docker build -t departure-ticker .
docker run --rm departure-ticker --demo --once
```

The image build runs warning checks, regression tests, and AddressSanitizer /
UndefinedBehaviorSanitizer tests. Tests use fixtures and a local HTTP server;
no API key, transit requests, or GPIO access are required.

Copy `.env.example` to `.env` and enter your API key locally. Then:

```sh
docker run --rm --env-file .env departure-ticker --once
docker run --rm --env-file .env departure-ticker
```

`--once` exits nonzero if fetching or parsing fails. `--demo` generates clearly
labelled simulated departures and needs no API key. Ctrl-C / SIGTERM shuts down
cleanly; an in-flight network request can take up to ten seconds to finish.

## Raspberry Pi 5 and Waveshare 64×32 panel

Build and run **on the Pi**, using a 64-bit Linux OS and Docker. Docker Desktop
on a Mac cannot drive the Pi's GPIO pins.

```sh
docker build --target matrix -t departure-ticker:matrix .
docker run --rm --privileged departure-ticker:matrix --demo \
  --led-rows=32 --led-cols=64 --led-chain=1 --led-brightness=50
```

Once the demo looks correct, start live departures:

```sh
docker run -d --name departure-ticker --restart unless-stopped \
  --privileged --env-file .env departure-ticker:matrix \
  --led-rows=32 --led-cols=64 --led-chain=1 --led-brightness=50
```

The hardware container needs GPIO/device access, hence `--privileged`. Stop it
with `docker stop departure-ticker`. The panel needs its own correctly rated
power supply and the correct HUB75 adapter/wiring.

The defaults are one 64×32 panel, regular GPIO mapping, and 50% brightness.
Waveshare sells several adapters and panel revisions: the exact hardware mapping,
scan/multiplexing, and GPIO slowdown must match your wiring. Pass the appropriate
upstream `--led-...=...` flags; the app forwards them to the driver. Pi 5 uses the
library's default RIO backend; PIO is available with `--led-rp1-pio=1` when its
kernel/device prerequisites are installed. See the
[upstream hardware and Pi 5 instructions](https://github.com/hzeller/rpi-rgb-led-matrix/tree/14464b3ad8974665f75a3c82355cf872311fec21).
Physical hardware operation still requires verification on your panel.

The bundled 5×7 font fits a heading and three stop rows. Pages rotate every four
seconds. Each row alternates between route and stop ID, with minutes remaining
at the right. `DUE` means arriving now; `--` means no upcoming prediction.
Failed updates replace predictions with `OFFLINE / RETRYING`, so old times are
not presented as live data. Long labels are clipped to the available width.

## Configuration

| Setting | Default / meaning |
| --- | --- |
| `API_KEY` | Required for live data; never logged |
| `STOP_IDS` | `0454,0133,0857,10062`; 1–10 distinct numeric IDs, comma-separated, no spaces |
| `POLL_SECONDS` | `15`; accepts 15–3600 seconds between update cycles |
| `API_FEED` | Optional `rtpidatafeed` name if required by your account |
| `MATRIX_FONT` | Optional path to a BDF font; container includes 5×7 |

Environment variables take precedence over `.env`, which takes precedence over
legacy `src/.env`. Dotenv accepts comments on separate lines, optional `export`,
whitespace around `=`, quoted values, and CRLF. It does not execute shell code,
expand variables, or support inline comments. Container `--env-file` uses
Docker's own parsing rules; use unquoted `NAME=value` assignments there.

API requests ask for Unix milliseconds to avoid DST ambiguity. Legacy local
strings are interpreted in `America/Chicago`, with daylight saving detection.
Impossible dates, malformed data, authentication/quota errors, and unknown API
errors fail the update. Recognized no-service responses produce empty stop rows.
Canceled, invalidated, and drop-off-only predictions are excluded. Predictions
more than 12 hours away or more than 59 seconds past are ignored. Within the
59-second grace period the display says `DUE` to accommodate minute-resolution
feeds. This app displays one next departure per stop, not every route at a stop.

Networking uses a five-second connection timeout, ten-second total timeout,
verified TLS, HTTP status checks, and a 2 MiB response limit. Failed updates retry
on the next cycle. Request time is additional to the configured interval.

## Development

Use the container toolchain by default:

```sh
docker build --target development -t departure-ticker:dev .
docker run --rm departure-ticker:dev make check
docker run --rm departure-ticker:dev make sanitize
```

To edit sources and check them inside that image:

```sh
docker run --rm -v "$PWD:/app" departure-ticker:dev \
  make BUILD_DIR=build/container check
```

If C11 tooling, pkg-config, libcurl, cJSON, Python 3, and timezone data are already
installed, native builds are also supported:

```sh
make check
./build/console/ticker --demo --once
make sanitize
```

For a native Pi matrix build, provide a built upstream matrix library:

```sh
make MATRIX=1 RPI_RGB=/path/to/rpi-rgb-led-matrix check
sudo MATRIX_FONT=/path/to/rpi-rgb-led-matrix/fonts/5x7.bdf \
  ./build/matrix/ticker --matrix --demo
```

`RPI_RGB` defaults to `/opt/rpi-rgb-led-matrix`. Console and matrix object files
are isolated in separate build directories. Header changes trigger rebuilding.
Ignore any old root-level `ticker` executable: current binaries live under
`build/console/` or `build/matrix/`.

Tests cover DST, Unix timestamps, route/time pairing, stale and canceled trips,
malformed responses, no-service vs API errors, URL escaping, long keys, dotenv,
stop validation, countdown rounding, HTTP failures/limits/timeouts, CLI demo and
shutdown. Matrix builds also test paging, labels, error display, and cleanup with
a simulated panel against the real driver header. Simulation does not verify
GPIO timing, wiring, power, or physical display quality.

## Credentials and dependencies

`.env` files and generated binaries are excluded from Git and the Docker context.
The previously tracked `src/.env` and build artifacts have been removed from the
index while retaining local copies. Removing tracking does **not** remove old
commits: rotate any credential previously committed through your Metro API
account. No history rewriting or remote credential changes are performed here.

The Dockerfile uses Debian Bookworm packages and pins the matrix library to
`14464b3ad8974665f75a3c82355cf872311fec21`. For distribution, review dependency
licenses, including the matrix library and its bundled font.

API behavior was checked against Madison's
[BusTime v3 guide, revision 3.30 (2025-12-19)](https://metromap.cityofmadison.com/bustime/apidoc/docs/DeveloperAPIGuide3_0.pdf).
