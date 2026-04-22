#!/bin/bash
args=()
for a in "$@"; do
  [[ "$a" == "--halt" ]] && continue
  args+=("$a")
done
exec /opt/st/stm32cubeclt_1.21.0/STLink-gdb-server/bin/ST-LINK_gdbserver "${args[@]}"
