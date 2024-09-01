FROM omzn/kit-lpp:v2

RUN useradd --shell /bin/bash --create-home devcontainer

COPY --chown=devcontainer:devcontainer <<EOF /home/devcontainer/.config/fish/conf.d/greeting.fish
    function fish_greeting
        cat /etc/motd
        cat /etc/issue
    end
EOF
