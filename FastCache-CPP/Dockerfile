FROM ubuntu:22.04 AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFASTCACHE_BUILD_TESTS=OFF \
    && cmake --build build -j"$(nproc)"

FROM ubuntu:22.04

RUN useradd --create-home --uid 10001 fastcache

WORKDIR /app
COPY --from=build /app/build/fastcache_server /app/fastcache_server

RUN mkdir -p /data && chown -R fastcache:fastcache /app /data

USER fastcache

EXPOSE 8080

ENTRYPOINT ["/app/fastcache_server"]
CMD ["--port", "8080", "--threads", "4", "--max-items", "100000", "--aof", "/data/appendonly.aof"]
