#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

require_symbol() {
    symbol=$1
    shift
    if ! grep -Eq "(^|[^[:alnum:]_])${symbol}([^[:alnum:]_]|$)" "$@"; then
        echo "FAIL: real-wiring symbol '${symbol}' is missing from $*" >&2
        exit 1
    fi
}

# Mechanical guard against the disconnected simulation recurring. The
# direct cpvt/at_enque call design superseded the original channel_tech
# draft in specifications section 9.7.
require_symbol gpublic "$ROOT/src/simbox_api.c"
require_symbol cpvt_alloc "$ROOT/src/simbox_modem.c"
require_symbol at_enque_dial "$ROOT/src/simbox_modem.c"
require_symbol at_enque_answer "$ROOT/src/simbox_modem.c"
require_symbol at_enque_hangup "$ROOT/src/simbox_modem.c"
require_symbol at_enque_sms "$ROOT/src/simbox_modem.c"
require_symbol at_enque_ussd "$ROOT/src/simbox_modem.c"
require_symbol at_enque_cmd_proc "$ROOT/src/simbox_modem.c"
require_symbol manager_event "$ROOT/adapters/src/shim_manager.c"
require_symbol simbox_at_response_bridge_capture "$ROOT/adapters/src/shim_logging.c"
echo "PASS: static real-wiring guard"

if [ "$(uname -s)" != "Linux" ]; then
    echo "SKIP: PTY wire smoke requires the real Linux branch"
    exit 0
fi
if ! command -v socat >/dev/null 2>&1; then
    echo "SKIP: socat is not installed" >&2
    exit 77
fi
if ! command -v timeout >/dev/null 2>&1; then
    echo "SKIP: timeout is not installed" >&2
    exit 77
fi

TMPDIR_WIRE=$(mktemp -d)
SOCAT_PID=
cleanup() {
    if [ -n "$SOCAT_PID" ]; then
        kill "$SOCAT_PID" 2>/dev/null || true
        wait "$SOCAT_PID" 2>/dev/null || true
    fi
    rm -rf "$TMPDIR_WIRE"
}
trap cleanup EXIT INT TERM

DRIVER_PTY="$TMPDIR_WIRE/driver"
PEER_PTY="$TMPDIR_WIRE/peer"
socat -d -d \
    "pty,raw,echo=0,link=$DRIVER_PTY" \
    "pty,raw,echo=0,link=$PEER_PTY" \
    >"$TMPDIR_WIRE/socat.log" 2>&1 &
SOCAT_PID=$!

i=0
while [ ! -e "$DRIVER_PTY" ] || [ ! -e "$PEER_PTY" ]; do
    i=$((i + 1))
    if [ "$i" -ge 50 ]; then
        echo "FAIL: socat did not create PTYs" >&2
        exit 1
    fi
    sleep 0.1
done

${CC:-cc} -O2 -Wall -DHAVE_CONFIG_H \
    -I"$ROOT/adapters/include" -I"$ROOT/src" \
    -I"$ROOT/asterisk_chan_svistok/chan_svistok" \
    "$ROOT/tests/test_at_wire.c" "$ROOT/libsimbox.a" \
    -lpthread -lm -o "$TMPDIR_WIRE/test_at_wire"

"$TMPDIR_WIRE/test_at_wire" "$DRIVER_PTY" &
HELPER_PID=$!
timeout 5 dd if="$PEER_PTY" of="$TMPDIR_WIRE/actual" bs=1 count=7 2>/dev/null
wait "$HELPER_PID"
printf 'AT+CSQ\r' >"$TMPDIR_WIRE/expected"
if ! cmp -s "$TMPDIR_WIRE/expected" "$TMPDIR_WIRE/actual"; then
    echo "FAIL: public simbox_at_command did not put expected bytes on PTY" >&2
    od -An -tx1 "$TMPDIR_WIRE/actual" >&2 || true
    exit 1
fi

echo "PASS: simbox_at_command emitted AT+CSQ\\r across socat PTY"
