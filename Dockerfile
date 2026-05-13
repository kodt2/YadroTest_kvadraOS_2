FROM ubuntu:22.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    curl cmake g++ zlib1g-dev libssl-dev nlohmann-json3-dev git \
    && curl -fsSL https://deb.nodesource.com/setup_20.x | bash - \
    && apt-get install -y nodejs

WORKDIR /app
COPY . .

RUN mkdir -p backend/build && cd backend/build && cmake .. && make -j$(nproc)
RUN mkdir -p public/src
RUN cd frontend && npm install && npx tsc --outDir ../public/
RUN cp frontend/*.html public/
RUN cp frontend/*.css public/


FROM ubuntu:22.04
RUN apt-get update && apt-get install -y libssl3 zlib1g && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/backend/build/monitor_backend .
COPY --from=builder /app/public ./public/src

EXPOSE 8080
CMD ["./monitor_backend"]