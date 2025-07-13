#!/bin/bash

# Create helpful aliases
cat >> ~/.zshrc << 'EOF'

# ==== DEVELOPMENT ENVIRONMENT SETUP ====

# Set up history
HISTSIZE=10000
SAVEHIST=10000
HISTFILE=~/.zsh_history
setopt SHARE_HISTORY
setopt HIST_IGNORE_DUPS
setopt HIST_IGNORE_ALL_DUPS
setopt HIST_SAVE_NO_DUPS
setopt HIST_IGNORE_SPACE
setopt HIST_VERIFY
setopt EXTENDED_HISTORY

# Enable autocompletion
autoload -Uz compinit
compinit

# Better tab completion
zstyle ':completion:*' menu select
zstyle ':completion:*' matcher-list 'm:{a-zA-Z}={A-Za-z}'

EOF

# Set up git configuration with better defaults
git config --global init.defaultBranch main
git config --global core.editor "code --wait"
git config --global diff.tool "vscode"
git config --global merge.tool "vscode"
git config --global difftool.vscode.cmd 'code --wait --diff $LOCAL $REMOTE'
git config --global mergetool.vscode.cmd 'code --wait $MERGED'

# Create a starship configuration for a nice prompt
mkdir -p ~/.config
cat > ~/.config/starship.toml << 'EOF'
# Starship configuration for Pico FreeRTOS development

format = """
[](#9A348E)\
$username\
[](bg:#DA627D fg:#9A348E)\
$directory\
[](fg:#FCA17D bg:#86BBD8)\
$c\
$cpp\
$rust\
[](fg:#86BBD8 bg:#06969A)\
$docker_context\
[](fg:#06969A bg:#33658A)\
$time\
[ ](fg:#33658A)\
"""

# Allow more time for commands to initilize
command_timeout = 1000

# Disable the blank line at the start of the prompt
add_newline = false

# You can also replace your username with a neat symbol like  to save some space
[username]
show_always = true
style_user = "bg:#9A348E"
style_root = "bg:#9A348E"
format = '[$user ]($style)'

[directory]
style = "bg:#DA627D"
format = "[ $path ]($style)"
truncation_length = 3
truncation_symbol = "…/"

[c]
symbol = " "
style = "bg:#86BBD8"
format = '[ $symbol ($version) ]($style)'

[cpp]
symbol = " "
style = "bg:#86BBD8"
format = '[ $symbol ($version) ]($style)'

[docker_context]
symbol = " "
style = "bg:#06969A"
format = '[ $symbol $context ]($style) $path'

[rust]
symbol = " "
style = "bg:#86BBD8"
format = '[ $symbol ($version) ]($style)'

[time]
disabled = false
time_format = "%R" # Hour:Minute Format
style = "bg:#33658A"
format = '[ ♥ $time ]($style)'
EOF
