FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install build tools and OpenGL/Mesa dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libglfw3-dev \
    libglew-dev \
    libglm-dev \
    mesa-utils \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libegl1-mesa-dev \
    mesa-common-dev \
    xvfb \
    x11vnc \
    novnc \
    websockify \
    openbox \
    xterm \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

# Build
RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

# Software rendering environment variables
ENV LIBGL_ALWAYS_SOFTWARE=1
ENV MESA_GL_VERSION_OVERRIDE=4.6
ENV MESA_GLSL_VERSION_OVERRIDE=460
ENV DISPLAY=:99

# Start script: Xvfb + window manager + VNC + app
RUN echo '#!/bin/bash\n\
Xvfb :99 -screen 0 1920x1080x24 &\n\
sleep 1\n\
openbox &\n\
x11vnc -display :99 -forever -nopw -shared -rfbport 5900 &\n\
websockify --web /usr/share/novnc 6080 localhost:5900 &\n\
echo ""\n\
echo "=== Mesh2Splat is running ==="\n\
echo "Open http://localhost:6080/vnc.html in your browser to view the GUI"\n\
echo ""\n\
cd /app && exec ./bin/Release/Mesh2Splat "$@"\n\
' > /start.sh && chmod +x /start.sh

EXPOSE 5900 6080

ENTRYPOINT ["/start.sh"]
