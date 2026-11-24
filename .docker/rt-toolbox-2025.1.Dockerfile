FROM ubuntu:24.04 AS base

RUN apt-get update && apt-get install -y curl

# Add certificates for collab compilers
RUN curl -fsSLo /usr/share/keyrings/rt-collab.gpg https://download.rt-labs.com/apt/rt-collab.gpg && \
    echo "deb [signed-by=/usr/share/keyrings/rt-collab.gpg arch=amd64] https://download.rt-labs.com/apt/rt-collab stable main" > /etc/apt/sources.list.d/rt-collab.list

RUN apt-get update && apt-get install -y \
        git \
        make \
        ninja-build \
        patch \
        python3 \
        python3-yaml \
        rt-toolbox-arm-eabi-gcc=13.3.0 \
        rt-kernel-arm9e=1.18.0 \
        rt-kernel-xmc4=1.18.0 \
        && \
    apt-get clean --yes

# Preinstall uv in our image
COPY --from=ghcr.io/astral-sh/uv:0.5.9 /uv /uvx /usr/local/bin/
ENV UV_LINK_MODE=copy

# Prepare for jenkins use, these will be overridden by jenkins during
# build to match the actual user building.
ARG BUILDER_UID=1001
ARG BUILDER_GID=1001
ARG BUILDER_USER=user
ARG BUILDER_GROUP=user
RUN groupadd -g ${BUILDER_GID} ${BUILDER_GROUP} && \
        useradd -rm \
                -d /home/${BUILDER_USER} \
                -s /bin/bash \
                -g ${BUILDER_GID} \
                -G ${BUILDER_GROUP} \
                -u ${BUILDER_UID} \
                ${BUILDER_USER}
USER ${BUILDER_USER}

ENV PATH=/home/${BUILDER_USER}/.local/bin:/opt/rt-tools/compilers/arm-eabi/bin:$PATH
