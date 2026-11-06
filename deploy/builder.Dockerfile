FROM ubuntu:24.04

RUN apt-get update \
    && apt-get install -y --no-install-recommends build-essential ninja-build perl pkg-config python3 python3-venv ca-certificates \
    && rm -rf /var/lib/apt/lists/*

RUN python3 -m venv /opt/venv
ENV PATH=/opt/venv/bin:$PATH
RUN pip install --no-cache-dir conan cmake && conan profile detect

WORKDIR /src
COPY conanfile.txt .
RUN conan install . --build=missing --output-folder=/conan -c tools.cmake.cmaketoolchain:generator=Ninja \
    && conan cache clean "*"

COPY . .
RUN cmake -S . -B /build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/conan/build/Release/generators/conan_toolchain.cmake \
    && cmake --build /build --target matching-engine orderbook-publisher candlestick-publisher execution-publisher order-manager archiver-process gateway
