# This file is auto-generated by openmediavault (https://www.openmediavault.org)
# WARNING: Do not edit this file, your changes will get lost.

# marslander-trainer
# Marslander AI trainer facility.

FROM gcc:latest

RUN apt-get update && apt-get install -y \
  git \
  cmake-data \
  cmake \
  libprotobuf-dev \
  protobuf-compiler \
&& rm -rf /var/lib/apt/lists/*

ADD entrypoint.sh /usr/local/bin
RUN chmod 544 /usr/local/bin/entrypoint.sh

ENV MARSLANDER_USER= < REDACTED > \
    MARSLANDER_TOKEN= < REDACTED >

WORKDIR /usr/src/marslander
ENTRYPOINT [ "/bin/bash", "-c", "entrypoint.sh master release /var/lib/marslander" ]
