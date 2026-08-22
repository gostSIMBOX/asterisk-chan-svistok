/* Linux-only helper for tests/test_real_wiring.sh.
 *
 * Builds a minimal real pvt around a socat PTY and invokes the public
 * simbox_at_command() entry point. No response pump is started here: the
 * public call is expected to time out after writing, while the shell test
 * independently proves the exact command bytes reached the peer PTY.
 */
#include "simbox_api.h"
#include "simbox_internal_linux.h"

#include <asterisk/linkedlists.h>
#include <chan_dongle.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    pvt_t pvt;
    simbox_device_t dev;
    char response[128];

    if (argc != 2) {
        fprintf(stderr, "usage: %s PTY\n", argv[0]);
        return 2;
    }

    memset(&pvt, 0, sizeof(pvt));
    strncpy(pvt.settings.unique.id, "wiretest",
            sizeof(pvt.settings.unique.id) - 1);
    strncpy(pvt.serial, "wiretest", sizeof(pvt.serial) - 1);
    pvt.data_fd = open(argv[1], O_RDWR | O_NOCTTY);
    if (pvt.data_fd < 0) {
        perror("open PTY");
        return 1;
    }
    AST_LIST_HEAD_INIT_NOLOCK(&pvt.at_queue);
    pvt.sys_chan.pvt = &pvt;

    dev = simbox_device_wrap_pvt(&pvt);
    if (!dev) {
        close(pvt.data_fd);
        return 1;
    }

    /* No reader thread is attached in this focused wire test, so timeout
     * is expected. The peer-side byte assertion is the pass condition. */
    (void)simbox_at_command(dev, "AT+CSQ", response, sizeof(response));
    simbox_device_destroy(dev);
    close(pvt.data_fd);
    return 0;
}
