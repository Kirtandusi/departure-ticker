FROM debian:bookworm-slim AS development
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential pkg-config libcurl4-openssl-dev libcjson-dev \
    ca-certificates tzdata python3 git && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY Makefile ./
COPY include/ include/
COPY src/ src/
COPY tests/ tests/
RUN make check && make sanitize

FROM development AS matrix-build
ARG MATRIX_COMMIT=14464b3ad8974665f75a3c82355cf872311fec21
RUN git init /opt/rpi-rgb-led-matrix && \
    git -C /opt/rpi-rgb-led-matrix remote add origin https://github.com/hzeller/rpi-rgb-led-matrix.git && \
    git -C /opt/rpi-rgb-led-matrix fetch --depth 1 origin "$MATRIX_COMMIT" && \
    git -C /opt/rpi-rgb-led-matrix checkout --detach FETCH_HEAD && \
    make -C /opt/rpi-rgb-led-matrix/lib
RUN make MATRIX=1 check

FROM debian:bookworm-slim AS runtime
RUN apt-get update && apt-get install -y --no-install-recommends \
    libcurl4 libcjson1 ca-certificates tzdata libstdc++6 && \
    rm -rf /var/lib/apt/lists/*
WORKDIR /app

FROM runtime AS matrix
COPY --from=matrix-build /app/build/matrix/ticker /usr/local/bin/ticker
COPY --from=matrix-build /opt/rpi-rgb-led-matrix/fonts/5x7.bdf /usr/local/share/departure-ticker/5x7.bdf
ENTRYPOINT ["ticker", "--matrix"]

FROM runtime AS console
COPY --from=development /app/build/console/ticker /usr/local/bin/ticker
USER 65534:65534
ENTRYPOINT ["ticker"]
