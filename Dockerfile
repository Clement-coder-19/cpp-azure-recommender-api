FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libasio-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

RUN git clone https://github.com/CrowCpp/Crow.git && \
    cd Crow && mkdir build && cd build && \
    cmake .. -DCROW_FEATURES=OFF && make install

COPY main.cpp .
COPY dataset/ ./dataset/

RUN g++ -O3 main.cpp -o recommender_api -lpthread -lasio

FROM ubuntu:24.04

WORKDIR /app

COPY --from=builder /app/recommender_api .
COPY --from=builder /app/dataset/ ./dataset/

EXPOSE 8080

CMD ["./recommender_api"]