FROM gcc:latest

WORKDIR /app

# Install dependencies
RUN apt-get update && \
    apt-get install -y make libprotobuf-c-dev libcurl4-openssl-dev

# Copy Makefile first (caching)
COPY Makefile .

# Copy include and proto headers (needed for compilation)
COPY include/ include/
COPY src/ src/

# Build
RUN make

# Set entrypoint if desired
CMD ["./bus_ticker"]
