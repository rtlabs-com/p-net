FROM dockcross/linux-arm64-lts
ENV DEFAULT_DOCKCROSS_IMAGE p-net-rbpi
RUN apt-get install -y python3-venv

RUN pip install --break-system-packages uv

ARG BUILDER_UID=1001
ARG BUILDER_GID=1001
ARG BUILDER_USER=rtljenkins
ARG BUILDER_GROUP=rtljenkins
RUN BUILDER_UID=${BUILDER_GID} BUILDER_GID=${BUILDER_GID} BUILDER_USER=${BUILDER_USER} BUILDER_GROUP=${BUILDER_GROUP} /dockcross/entrypoint.sh echo "Setup jenkins user"
