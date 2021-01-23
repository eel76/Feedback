# See here for image contents: https://github.com/microsoft/vscode-dev-containers/tree/v0.154.2/containers/cpp/.devcontainer/base.Dockerfile

# [Choice] Debian / Ubuntu version: debian-10, debian-9, ubuntu-20.04, ubuntu-18.04
ARG VARIANT="buster"
FROM mcr.microsoft.com/vscode/devcontainers/cpp:0-${VARIANT}

# [Optional] Uncomment this section to install additional packages.
# RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
#     && apt-get -y install --no-install-recommends <your-package-list-here>

RUN apt-get update \
&& apt-get -y remove --purge --auto-remove cmake \
&& apt-get -y install libssl-dev libtbb-dev \
&& version=3.18 \
&& build=5 \
&& mkdir ~/temp \
&& cd ~/temp \
&& wget https://cmake.org/files/v$version/cmake-$version.$build.tar.gz \
&& tar -xzvf cmake-$version.$build.tar.gz \
&& cd cmake-$version.$build \
&& ./bootstrap \
&& make -j$(nproc) \
&& make install \
&& cd ~ \
&& rm -rf ~/temp
