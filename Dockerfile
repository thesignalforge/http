# Dockerfile for building and testing Signalforge HTTP extension
# Mirrors php-builds approach: compiles PHP from source then builds extension
#
# Build args:
#   VERSION - PHP version (83, 84, 85) - defaults to 85 (PHP 8.5)
#
ARG VERSION=85

FROM ubuntu:24.04

ARG VERSION
ENV DEBIAN_FRONTEND=noninteractive
ENV PHP_VERSION=${VERSION}

LABEL maintainer="Signalforge Team"
LABEL description="PHP with signalforge_http extension"
LABEL php.version="${PHP_VERSION}"

# Install build dependencies
RUN apt-get update -qq && apt-get install -y -qq --no-install-recommends \
    git \
    gcc \
    g++ \
    make \
    autoconf \
    automake \
    libtool \
    pkg-config \
    re2c \
    bison \
    wget \
    libxml2-dev \
    libssl-dev \
    libcurl4-openssl-dev \
    libzip-dev \
    libonig-dev \
    libsqlite3-dev \
    libpq-dev \
    libreadline-dev \
    libpcre2-dev \
    libsodium-dev \
    zlib1g-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Clone and build PHP from source
WORKDIR /tmp
RUN MAJOR=$(echo ${PHP_VERSION} | cut -c1); \
    MINOR=$(echo ${PHP_VERSION} | cut -c2); \
    PHP_BRANCH="PHP-${MAJOR}.${MINOR}"; \
    git clone --depth 1 --branch ${PHP_BRANCH} --quiet https://github.com/php/php-src.git

WORKDIR /tmp/php-src
RUN ./buildconf --force > /dev/null

RUN ./configure --quiet \
    --prefix=/usr/local \
    --with-config-file-path=/usr/local/etc/php \
    --with-config-file-scan-dir=/usr/local/etc/php/conf.d \
    --with-curl \
    --with-openssl \
    --with-zip \
    --with-zlib \
    --enable-mbstring \
    --enable-opcache \
    --with-pdo-mysql \
    --with-pdo-pgsql \
    --with-mysqli \
    --enable-sockets \
    --enable-pcntl \
    --enable-bcmath \
    --with-readline

RUN make -j$(nproc)
RUN make install

# Create extension config directory
RUN mkdir -p /usr/local/etc/php/conf.d

# Copy and build http extension
WORKDIR /build
COPY . /build

RUN /usr/local/bin/phpize \
    && ./configure --enable-signalforge_http \
    && make -j$(nproc) \
    && make install

# Enable extension
RUN echo "extension=signalforge_http.so" > /usr/local/etc/php/conf.d/signalforge_http.ini

# Get run-tests.php from PHP source
RUN wget -q -O /opt/run-tests.php https://raw.githubusercontent.com/php/php-src/master/run-tests.php

# Verify extension is loaded
RUN php -m | grep signalforge_http

WORKDIR /ext
CMD ["php", "-v"]

