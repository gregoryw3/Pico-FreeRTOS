FROM ubuntu:24.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    pkg-config \
    libusb-1.0-0-dev \
    cmake \
    git \
    wget \
    curl \
    zsh \
    tree \
    htop \
    nano \
    vim \
    unzip \
    fontconfig \
    python3 \
    python3-pip \
    python3-venv \
    && rm -rf /var/lib/apt/lists/*

# Install FiraCode Nerd Font
RUN mkdir -p /usr/share/fonts/truetype/firacode \
    && wget https://github.com/ryanoasis/nerd-fonts/releases/download/v3.1.1/FiraCode.zip -O /tmp/FiraCode.zip \
    && unzip /tmp/FiraCode.zip -d /tmp/FiraCode \
    && cp /tmp/FiraCode/*.ttf /usr/share/fonts/truetype/firacode/ \
    && fc-cache -f -v \
    && rm -rf /tmp/FiraCode.zip /tmp/FiraCode

# Detect architecture and download the correct ARM GCC toolchain
RUN if [ "$(uname -m)" = "aarch64" ]; then \
    wget https://developer.arm.com/-/media/Files/downloads/gnu/14.3.rel1/binrel/arm-gnu-toolchain-14.3.rel1-aarch64-arm-none-eabi.tar.xz && \
    tar -xf arm-gnu-toolchain-14.3.rel1-aarch64-arm-none-eabi.tar.xz && \
    rm arm-gnu-toolchain-14.3.rel1-aarch64-arm-none-eabi.tar.xz && \
    mv arm-gnu-toolchain-14.3.rel1-aarch64-arm-none-eabi /usr/local/arm-none-eabi; \
    else \
    wget https://developer.arm.com/-/media/Files/downloads/gnu/14.3.rel1/binrel/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi.tar.xz && \
    tar -xf arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi.tar.xz && \
    rm arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi.tar.xz && \
    mv arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi /usr/local/arm-none-eabi; \
    fi

# Add ARM toolchain to PATH
ENV PATH="/usr/local/arm-none-eabi/bin:${PATH}"

# Install Starship prompt
RUN curl -sS https://starship.rs/install.sh | sh -s -- -y

# Create vscode user and configure sudo
RUN useradd -m vscode -s /bin/zsh \
    && apt-get update && apt-get install -y sudo \
    && echo "vscode ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers.d/90-vscode \
    && chmod 0440 /etc/sudoers.d/90-vscode \
    && rm -rf /var/lib/apt/lists/*

# Print ARM GCC toolchain version
RUN arm-none-eabi-gcc -v
# Should be: gcc version 14.3.1 20250623 (Arm GNU Toolchain 14.3.Rel1 (Build arm-14.174))

# Switch to vscode user
USER vscode

# Set up zsh and starship for vscode user
RUN echo 'eval "$(starship init zsh)"' >> ~/.zshrc

# Copy shell setup script and make it executable
COPY --chown=vscode:vscode .devcontainer/setup-shell.sh /home/vscode/setup-shell.sh
RUN chmod +x /home/vscode/setup-shell.sh && /home/vscode/setup-shell.sh && rm /home/vscode/setup-shell.sh

# Set up the working directory
WORKDIR /workspace