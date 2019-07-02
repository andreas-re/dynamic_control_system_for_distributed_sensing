/*
 This example code is in the Public Domain (or CC0 licensed, at your option.)

 Unless required by applicable law or agreed to in writing, this
 software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied.
 */

/****************************************************************************
 *
 * This file is used for eddystone receiver.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "stdbool.h"

#include "esp_bt.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_defs.h"
#include "esp_gattc_api.h"
#include "esp_gap_ble_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_eddystone_protocol.h"
#include "esp_eddystone_api.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

static const char* TAG = "EDDYSTONE_BLE";

#define HCI_H4_CMD_PREAMBLE_SIZE           (4)

/*  HCI Command opcode group field(OGF) */
#define HCI_GRP_HOST_CONT_BASEBAND_CMDS    (0x03 << 10)            /* 0x0C00 */
#define HCI_GRP_BLE_CMDS                   (0x08 << 10)

#define HCI_RESET                          (0x0003 | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_BLE_WRITE_ADV_ENABLE           (0x000A | HCI_GRP_BLE_CMDS)
#define HCI_BLE_WRITE_ADV_PARAMS           (0x0006 | HCI_GRP_BLE_CMDS)
#define HCI_BLE_WRITE_ADV_DATA             (0x0008 | HCI_GRP_BLE_CMDS)

#define HCIC_PARAM_SIZE_WRITE_ADV_ENABLE        (1)
#define HCIC_PARAM_SIZE_BLE_WRITE_ADV_PARAMS    (15)
#define HCIC_PARAM_SIZE_BLE_WRITE_ADV_DATA      (31)

#define BD_ADDR_LEN     (6)                     /* Device address length */
typedef uint8_t bd_addr_t[BD_ADDR_LEN];         /* Device address */

#define UINT16_TO_STREAM(p, u16) {*(p)++ = (uint8_t)(u16); *(p)++ = (uint8_t)((u16) >> 8);}
#define UINT8_TO_STREAM(p, u8)   {*(p)++ = (uint8_t)(u8);}
#define BDADDR_TO_STREAM(p, a)   {int ijk; for (ijk = 0; ijk < BD_ADDR_LEN;  ijk++) *(p)++ = (uint8_t) a[BD_ADDR_LEN - 1 - ijk];}
#define ARRAY_TO_STREAM(p, a, len) {int ijk; for (ijk = 0; ijk < len;        ijk++) *(p)++ = (uint8_t) a[ijk];}

/* declare static functions */
static void esp_gap_cb(esp_gap_ble_cb_event_t event,
		esp_ble_gap_cb_param_t* param);
static void esp_eddystone_show_inform(const esp_eddystone_result_t* res,
		lua_State *L);

static lua_State *L1;
char *uid = "UyiNJXcSzXwfBCQIvwcB";

static esp_ble_scan_params_t ble_scan_params = { .scan_type =
		BLE_SCAN_TYPE_ACTIVE, .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
		.scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL, .scan_interval = 0x50,
		.scan_window = 0x30 };

void addTableInt(lua_State *L, char *index, int value) {
	lua_pushstring(L, index); /* key */
	lua_pushnumber(L, value); /* value */
	lua_settable(L, -3);
}

void addTableString(lua_State *L, char *index, const char *value) {
	lua_pushstring(L, index); /* key */
	lua_pushstring(L, value); /* value */
	lua_settable(L, -3);
}

static void esp_eddystone_show_inform(const esp_eddystone_result_t* res,
		lua_State *L) {
	switch (res->common.frame_type) {
	case EDDYSTONE_FRAME_TYPE_UID: {
		ESP_LOGI(TAG, "Eddystone UID inform:");
		addTableString(L, "type", "UID");
		ESP_LOGI(TAG, "Measured power(RSSI at 0m distance):%d dbm",
				res->inform.uid.ranging_data);
//            addTableInt(L, "power", res->inform.uid.ranging_data);
		ESP_LOGI(TAG, "Namespace ID:0x");
		esp_log_buffer_hex(TAG, res->inform.uid.namespace_id, 10);
//            addTableInt(L, "namespaceId", res->inform.uid.namespace_id);
		ESP_LOGI(TAG, "Instance ID:0x");
		esp_log_buffer_hex(TAG, res->inform.uid.instance_id, 6);
//            addTableInt(L, "instanceId", res->inform.uid.instance_id);
		break;
	}
	case EDDYSTONE_FRAME_TYPE_URL: {
		ESP_LOGI(TAG, "Eddystone URL inform:");
		addTableString(L, "type", "URL");
		ESP_LOGI(TAG, "Measured power(RSSI at 0m distance):%d dbm",
				res->inform.url.tx_power);
//            addTableInt(L, "power", res->inform.url.tx_power);
		ESP_LOGI(TAG, "URL: %s", res->inform.url.url);
//            addTableString(L, "url", res->inform.url.url);
		break;
	}
	case EDDYSTONE_FRAME_TYPE_TLM: {
		ESP_LOGI(TAG, "Eddystone TLM inform:");
		addTableString(L, "type", "TLM");
		ESP_LOGI(TAG, "version: %d", res->inform.tlm.version);
//            addTableInt(L, "version", res->inform.tlm.version);
		ESP_LOGI(TAG, "battery voltage: %d mV", res->inform.tlm.battery_voltage);
//            addTableInt(L, "voltage", res->inform.tlm.battery_voltage);
		ESP_LOGI(TAG, "beacon temperature in degrees Celsius: %6.1f",
				res->inform.tlm.temperature);
//            addTableInt(L, "temterature", res->inform.tlm.temperature);
		ESP_LOGI(TAG, "adv pdu count since power-up: %d",
				res->inform.tlm.adv_count);
//            addTableInt(L, "adv_count", res->inform.tlm.adv_count);
		ESP_LOGI(TAG, "time since power-up: %d s", res->inform.tlm.time);
//            addTableInt(L, "time", res->inform.tlm.time);
		break;
	}
	default:
		break;
	}
}

static void esp_gap_cb(esp_gap_ble_cb_event_t event,
		esp_ble_gap_cb_param_t* param) {
	switch (event) {
	case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
		uint32_t duration = 0;
		esp_ble_gap_start_scanning(duration);
		break;
	}
	case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT: {
		if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
			ESP_LOGE(TAG, "Scan start failed");
		} else {
			ESP_LOGI(TAG, "Start scanning...");
		}
		break;
	}
	case ESP_GAP_BLE_SCAN_RESULT_EVT: {
		esp_ble_gap_cb_param_t* scan_result = (esp_ble_gap_cb_param_t*) param;
		switch (scan_result->scan_rst.search_evt) {
		case ESP_GAP_SEARCH_INQ_RES_EVT: {
			esp_eddystone_result_t eddystone_res;
			memset(&eddystone_res, 0, sizeof(eddystone_res));
			esp_err_t ret = esp_eddystone_decode(scan_result->scan_rst.ble_adv,
					scan_result->scan_rst.adv_data_len, &eddystone_res);
			if (ret) {
				// error:The received data is not an eddystone frame packet or a correct eddystone frame packet.
				// just return
				return;
			} else {
				// The received adv data is a correct eddystone frame packet.
				// Here, we get the eddystone infomation in eddystone_res, we can use the data in res to do other things.
				// For example, just print them:
				lua_getglobal(L1, uid);
				lua_newtable(L1);
				ESP_LOGI(TAG, "--------Eddystone Found----------");
				esp_log_buffer_hex("EDDYSTONE_BLE: Device address:", scan_result->scan_rst.bda, ESP_BD_ADDR_LEN);
//                        addTableInt(L1, "device", scan_result->scan_rst.bda);
				ESP_LOGI(TAG, "RSSI of packet:%d dbm",
						scan_result->scan_rst.rssi);
//                        addTableInt(L1, "rssi", scan_result->scan_rst.rssi);
				esp_eddystone_show_inform(&eddystone_res, L1);
				lua_pcall(L1, 1, 0, 0);
			}
			break;
		}
		default:
			break;
		}
		break;
	}
	case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT: {
		if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
			ESP_LOGE(TAG, "Scan stop failed");
		} else {
			ESP_LOGI(TAG, "Stop scan successfully");
		}
		break;
	}
	default:
		break;
	}
}

void esp_eddystone_appRegister(void) {
	esp_err_t status;

	ESP_LOGI(TAG, "Register callback");

	/*<! register the scan callback function to the gap module */
	if ((status = esp_ble_gap_register_callback(esp_gap_cb)) != ESP_OK) {
		ESP_LOGE(TAG, "gap register error,error code = %x", status);
		return;
	}
}

void esp_eddystone_init(void) {
	esp_bluedroid_init();
	esp_bluedroid_enable();
	esp_eddystone_appRegister();
}

int eddystone_init(lua_State *L) {
	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT()
	;
	esp_bt_controller_init(&bt_cfg);
	esp_bt_controller_enable(ESP_BT_MODE_BLE);

	esp_eddystone_init();
	return 0;
}

int eddystone_run(lua_State *L) {
	//create new lua thread for callbacks
	lua_settop(L, 1);
	L1 = L;
	lua_pushvalue(L1, 1);
	lua_setglobal(L1, uid);

	/*<! set scan parameters */
	esp_ble_gap_set_scan_params(&ble_scan_params);

	return 0;
}

int eddystone_stop(lua_State *L) {
	esp_ble_gap_stop_scanning();
	return 0;
}

enum {
    H4_TYPE_COMMAND = 1,
    H4_TYPE_ACL     = 2,
    H4_TYPE_SCO     = 3,
    H4_TYPE_EVENT   = 4
};

static uint8_t hci_cmd_buf[128];

/*
 * @brief: BT controller callback function, used to notify the upper layer that
 *         controller is ready to receive command
 */
static void controller_rcv_pkt_ready(void)
{
    printf("controller rcv pkt ready\n");
}

/*
 * @brief: BT controller callback function, to transfer data packet to upper
 *         controller is ready to receive command
 */
static int host_rcv_pkt(uint8_t *data, uint16_t len)
{
    printf("host rcv pkt: ");
    for (uint16_t i=0; i<len; i++)
        printf("%02x", data[i]);
    printf("\n");
    return 0;
}

static esp_vhci_host_callback_t vhci_host_cb = {
    controller_rcv_pkt_ready,
    host_rcv_pkt
};

static uint16_t make_cmd_reset(uint8_t *buf)
{
    UINT8_TO_STREAM (buf, H4_TYPE_COMMAND);
    UINT16_TO_STREAM (buf, HCI_RESET);
    UINT8_TO_STREAM (buf, 0);
    return HCI_H4_CMD_PREAMBLE_SIZE;
}

static uint16_t make_cmd_ble_set_adv_enable (uint8_t *buf, uint8_t adv_enable)
{
    UINT8_TO_STREAM (buf, H4_TYPE_COMMAND);
    UINT16_TO_STREAM (buf, HCI_BLE_WRITE_ADV_ENABLE);
    UINT8_TO_STREAM  (buf, HCIC_PARAM_SIZE_WRITE_ADV_ENABLE);
    UINT8_TO_STREAM (buf, adv_enable);
    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_ADV_ENABLE;
}

static uint16_t make_cmd_ble_set_adv_param (uint8_t *buf, uint16_t adv_int_min, uint16_t adv_int_max,
                                            uint8_t adv_type, uint8_t addr_type_own,
                                            uint8_t addr_type_dir, bd_addr_t direct_bda,
                                            uint8_t channel_map, uint8_t adv_filter_policy)
{
    UINT8_TO_STREAM (buf, H4_TYPE_COMMAND);
    UINT16_TO_STREAM (buf, HCI_BLE_WRITE_ADV_PARAMS);
    UINT8_TO_STREAM  (buf, HCIC_PARAM_SIZE_BLE_WRITE_ADV_PARAMS );

    UINT16_TO_STREAM (buf, adv_int_min);
    UINT16_TO_STREAM (buf, adv_int_max);
    UINT8_TO_STREAM (buf, adv_type);
    UINT8_TO_STREAM (buf, addr_type_own);
    UINT8_TO_STREAM (buf, addr_type_dir);
    BDADDR_TO_STREAM (buf, direct_bda);
    UINT8_TO_STREAM (buf, channel_map);
    UINT8_TO_STREAM (buf, adv_filter_policy);
    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_BLE_WRITE_ADV_PARAMS;
}


static uint16_t make_cmd_ble_set_adv_data(uint8_t *buf, uint8_t data_len, uint8_t *p_data)
{
    UINT8_TO_STREAM (buf, H4_TYPE_COMMAND);
    UINT16_TO_STREAM (buf, HCI_BLE_WRITE_ADV_DATA);
    UINT8_TO_STREAM  (buf, HCIC_PARAM_SIZE_BLE_WRITE_ADV_DATA + 1);

    memset(buf, 0, HCIC_PARAM_SIZE_BLE_WRITE_ADV_DATA);

    if (p_data != NULL && data_len > 0) {
        if (data_len > HCIC_PARAM_SIZE_BLE_WRITE_ADV_DATA) {
            data_len = HCIC_PARAM_SIZE_BLE_WRITE_ADV_DATA;
        }

        UINT8_TO_STREAM (buf, data_len);

        ARRAY_TO_STREAM (buf, p_data, data_len);
    }
    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_BLE_WRITE_ADV_DATA + 1;
}

static void hci_cmd_send_reset(void)
{
    uint16_t sz = make_cmd_reset (hci_cmd_buf);
    esp_vhci_host_send_packet(hci_cmd_buf, sz);
}

static void hci_cmd_send_ble_adv_start(void)
{
    uint16_t sz = make_cmd_ble_set_adv_enable (hci_cmd_buf, 1);
    esp_vhci_host_send_packet(hci_cmd_buf, sz);
 }

static void hci_cmd_send_ble_set_adv_param(void)
{
    uint16_t adv_intv_min = 256; // 160ms
    uint16_t adv_intv_max = 256; // 160ms
    uint8_t adv_type = 0; // connectable undirected advertising (ADV_IND)
    uint8_t own_addr_type = 0; // Public Device Address
    uint8_t peer_addr_type = 0; // Public Device Address
    uint8_t peer_addr[6] = {0x80, 0x81, 0x82, 0x83, 0x84, 0x85};
    uint8_t adv_chn_map = 0x07; // 37, 38, 39
    uint8_t adv_filter_policy = 0; // Process All Conn and Scan

    uint16_t sz = make_cmd_ble_set_adv_param(hci_cmd_buf,
                                             adv_intv_min,
                                             adv_intv_max,
                                             adv_type,
                                             own_addr_type,
                                             peer_addr_type,
                                             peer_addr,
                                             adv_chn_map,
                                             adv_filter_policy);
    esp_vhci_host_send_packet(hci_cmd_buf, sz);
}

static void hci_cmd_send_ble_set_adv_data(void)
{
    uint8_t adv_data[31];
    uint8_t adv_data_len;

    adv_data[0] = 2;      // Len
    adv_data[1] = 0x01;   // Type Flags
    adv_data[2] = 0x06;   // GENERAL_DISC_MODE 0x02 | BR_EDR_NOT_SUPPORTED 0x04
    adv_data[3] = 3;      // Len
    adv_data[4] = 0x03;   // Type 16-Bit UUID
    adv_data[5] = 0xAA;   // Eddystone UUID 2 -> 0xFEAA LSB
    adv_data[6] = 0xFE;   // Eddystone UUID 1 MSB
    adv_data[7] = 19;     // Length of Beacon Data
    adv_data[8] = 0x16;   // Type Service Data
    adv_data[9] = 0xAA;   // Eddystone UUID 2 -> 0xFEAA LSB
    adv_data[10] = 0xFE;  // Eddystone UUID 1 MSB
    adv_data[11] = 0x10;  // Eddystone Frame Type
    adv_data[12] = 0x20;  // Beacons TX power at 0m
    adv_data[13] = 0x03;  // URL Scheme 'https://'
    adv_data[14] = 0x67;  // URL add  1 'g'
    adv_data[15] = 0x6F;  // URL add  2 'o'
    adv_data[16] = 0x6F;  // URL add  3 'o'
    adv_data[17] = 0x2E;  // URL add  4 '.'
    adv_data[18] = 0x67;  // URL add  5 'g'
    adv_data[19] = 0x6C;  // URL add  6 'l'
    adv_data[20] = 0x2F;  // URL add  7 '/'
    adv_data[21] = 0x4A;  // URL add  8 'J'
    adv_data[22] = 0x4B;  // URL add  9 'K'
    adv_data[23] = 0x70;  // URL add 10 'p'
    adv_data[24] = 0x6E;  // URL add 11 'n'
    adv_data[25] = 0x4C;  // URL add 12 'L'
    adv_data[26] = 0x44;  // URL add 13 'D'

    adv_data_len = 27;

    printf("Eddystone adv_data [%d]=",adv_data_len);
    for (int i=0; i<adv_data_len; i++) {
        printf("%02x",adv_data[i]);
    }
    printf("\n");


    uint16_t sz = make_cmd_ble_set_adv_data(hci_cmd_buf, adv_data_len, (uint8_t *)adv_data);
    esp_vhci_host_send_packet(hci_cmd_buf, sz);
}

/*
 * @brief: send HCI commands to perform BLE advertising;
 */
void bleAdvtTask()
{
    int cmd_cnt = 0;
    bool send_avail = false;
    esp_vhci_host_register_callback(&vhci_host_cb);
    printf("BLE advt task start\n");
    while (1) {
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        send_avail = esp_vhci_host_check_send_available();
        if (send_avail) {
            switch (cmd_cnt) {
            case 0: hci_cmd_send_reset(); ++cmd_cnt; break;
            case 1: hci_cmd_send_ble_set_adv_param(); ++cmd_cnt; break;
            case 2: hci_cmd_send_ble_set_adv_data(); ++cmd_cnt; break;
            case 3: hci_cmd_send_ble_adv_start(); ++cmd_cnt; break;
            }
        }
        printf("BLE Advertise, flag_send_avail: %d, cmd_sent: %d\n", send_avail, cmd_cnt);
    }
}

int eddystone_share(lua_State *L) {
	esp_err_t ret;

	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT()	;
	ret = esp_bt_controller_init(&bt_cfg);
	if (ret) {
		ESP_LOGE(TAG, "%s initialize controller failed\n", __func__);
		return 1;
	}

	ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
	if (ret) {
		ESP_LOGE(TAG, "%s enable controller failed\n", __func__);
		return 1;
	}

	bleAdvtTask();
	return 0;
}

static const struct luaL_Reg eddystone[] = {
		{ "init", eddystone_init },
		{ "share", eddystone_share},
		{ "start", eddystone_run },
		{ "stop", eddystone_stop },
		{ NULL, NULL } /* sentinel */
};

int luaopen_eddystone(lua_State *L) {
	luaL_newlib(L, eddystone);
	return 1;
}
