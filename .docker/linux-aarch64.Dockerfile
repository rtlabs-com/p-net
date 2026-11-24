FROM dockcross/linux-arm64
ENV DEFAULT_DOCKCROSS_IMAGE=p-net-linux-aarch64

# Preinstall uv in our image
COPY --from=ghcr.io/astral-sh/uv:0.5.9 /uv /uvx /usr/local/bin/
ENV UV_LINK_MODE=copy

# Prepare for jenkins use, these will
# be overridden by jenkins during build
# to match the actual user building.
ARG BUILDER_UID=1001
ARG BUILDER_GID=1001
ARG BUILDER_USER=rtljenkins
ARG BUILDER_GROUP=rtljenkins
RUN BUILDER_UID=${BUILDER_GID} BUILDER_GID=${BUILDER_GID} BUILDER_USER=${BUILDER_USER} BUILDER_GROUP=${BUILDER_GROUP} /dockcross/entrypoint.sh echo "Setup jenkins user"
