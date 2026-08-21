/*
 * Simbox Native SDK - Integration Test Suite
 */
#include "simbox_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int g_event_count = 0;

static void test_event_handler(const simbox_event_t *event, void *userdata)
{
    printf("[EVENT] Received event type: %d on device %s\n",
           event->type, event->device_sn ? event->device_sn : "N/A");
    g_event_count++;
}

static void test_lifecycle(void)
{
    printf("\n=== Test 1: SDK Lifecycle ===\n");
    simbox_config_t cfg = {
        .config_dir = "/tmp/simbox_test/etc",
        .state_dir = "/tmp/simbox_test/state",
        .log_level = 3,
        .auto_discovery = true,
        .auto_recover_diag = true
    };

    simbox_handle_t handle = simbox_init(&cfg);
    assert(handle != NULL);
    printf("SDK initialized successfully. Version: %s\n", simbox_version());

    simbox_set_event_callback(handle, test_event_handler, NULL);
    assert(simbox_device_count(handle) == 0);

    simbox_shutdown(handle);
    printf("SDK shutdown successfully.\n");
}

static void test_device_operations(void)
{
    printf("\n=== Test 2: Device Operations ===\n");
    simbox_handle_t handle = simbox_init(NULL);
    assert(handle != NULL);

    extern simbox_device_t simbox_device_create(const simbox_device_info_t *info);
    simbox_device_info_t info = {
        .sn = "TEST_SN_12345",
        .imei = "864321012345678",
        .imsi = "250010123456789",
        .name = "dongle0",
        .model = "E173",
        .firmware = "11.126.15.00.00",
        .rssi = 21,
        .state = SIMBOX_STATE_IDLE
    };

    simbox_device_t dev = simbox_device_create(&info);
    assert(dev != NULL);

    assert(strcmp(simbox_device_sn(dev), "TEST_SN_12345") == 0);
    assert(strcmp(simbox_device_imei(dev), "864321012345678") == 0);
    assert(strcmp(simbox_device_imsi(dev), "250010123456789") == 0);
    assert(simbox_device_state(dev) == SIMBOX_STATE_IDLE);
    assert(simbox_device_rssi(dev) == 21);

    /* Test AT command */
    char resp[128];
    int res = simbox_at_command(dev, "AT+CSQ\r", resp, sizeof(resp));
    assert(res == 0);
    assert(strstr(resp, "OK") != NULL);

    /* Test IMEI Change */
    res = simbox_change_imei(dev, "869999999999999");
    assert(res == 0);
    assert(strcmp(simbox_device_imei(dev), "869999999999999") == 0);

    /* Test Call lifecycle */
    res = simbox_call_originate(dev, "+1234567890");
    assert(res == 0);
    assert(simbox_device_state(dev) == SIMBOX_STATE_DIALING);

    res = simbox_call_answer(dev);
    assert(res == 0);
    assert(simbox_device_state(dev) == SIMBOX_STATE_ACTIVE_CALL);

    res = simbox_call_hangup(dev);
    assert(res == 0);
    assert(simbox_device_state(dev) == SIMBOX_STATE_IDLE);

    /* Test SMS & USSD */
    res = simbox_sms_send(dev, "+1234567890", "Hello from Simbox SDK");
    assert(res == 0);

    res = simbox_ussd_send(dev, "*100#");
    assert(res == 0);

    extern void simbox_device_destroy(simbox_device_t dev);
    simbox_device_destroy(dev);
    simbox_shutdown(handle);
    printf("Device operations test passed.\n");
}

static void test_discovery(void)
{
    printf("\n=== Test 3: Node Discovery ===\n");
    simbox_discovery_t disc = simbox_discovery_start(NULL);
    assert(disc != NULL);

    int count = simbox_discovery_scan(disc);
    printf("Discovery scan completed: %d devices found.\n", count);

    simbox_discovery_stop(disc);
    printf("Discovery test passed.\n");
}

static void test_programmator(void)
{
    printf("\n=== Test 4: Qualcomm DIAG Programmator ===\n");
    simbox_prog_t prog = simbox_prog_open(NULL);
    assert(prog != NULL);

    int res = simbox_prog_flash(prog, "1-1.2", "/tmp/firmware.bin", NULL, NULL);
    assert(res == 0);
    assert(simbox_prog_get_progress(prog) == 100);
    assert(strcmp(simbox_prog_get_state(prog), "SUCCESS") == 0);

    simbox_prog_close(prog);
    printf("Programmator test passed.\n");
}

static void test_reader(void)
{
    printf("\n=== Test 5: APDU SIM Reader ===\n");
    simbox_reader_t reader = simbox_reader_open(NULL);
    assert(reader != NULL);

    char atr[128];
    int res = simbox_reader_get_atr(reader, atr, sizeof(atr));
    assert(res == 0);
    printf("Reader ATR: %s\n", atr);
    assert(strlen(atr) > 0);

    uint8_t apdu[] = { 0xA0, 0xA4, 0x00, 0x00, 0x02, 0x3F, 0x00 };
    uint8_t resp[256];
    size_t resp_len = 0;
    res = simbox_reader_send_apdu(reader, apdu, sizeof(apdu), resp, &resp_len);
    assert(res == 0);
    assert(resp_len == 2);
    assert(resp[0] == 0x90 && resp[1] == 0x00);

    simbox_reader_close(reader);
    printf("APDU Reader test passed.\n");
}

int main(void)
{
    printf("========================================\n");
    printf("Starting Simbox Native SDK Test Suite\n");
    printf("========================================\n");

    test_lifecycle();
    test_device_operations();
    test_discovery();
    test_programmator();
    test_reader();

    printf("\n========================================\n");
    printf("ALL 5 INTEGRATION TEST SUITES PASSED!\n");
    printf("========================================\n");
    return 0;
}
