# Departure ticker development

- This is a C11 Madison Metro API client, with optional Raspberry Pi matrix output.
- Prefer the existing Docker workflow. Do not install host packages.
- `docker build --target development -t departure-ticker:dev .` builds and runs
  `make check` and `make sanitize`. `docker build --target matrix` additionally
  builds the pinned Pi 5-capable driver and runs simulated matrix tests.
- `make check` runs warnings-as-errors, core tests, and offline localhost HTTP/CLI
  tests. Matrix builds use `make MATRIX=1 RPI_RGB=... check`.
- Console and matrix artifacts live in `build/console` and `build/matrix`.
  Never use or commit legacy root-level binaries or source-directory objects.
- Test without credentials using `--demo --once`. Never print dotenv contents,
  API keys, request URLs, or raw API error bodies. Do not put credentials in images.
- Initialize Chicago timezone before starting matrix threads. Request absolute
  API timestamps; preserve route and time together when selecting departures.
- Hardware defaults: user has a Pi 5 and one Waveshare 64x32 panel; exact adapter
  mapping is unconfirmed. Simulated tests do not establish physical operation.
