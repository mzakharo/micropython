/********************************************************************************
	* Copyright (C), 2022 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name				:	mod_wifllink.c
	* Author				:	Folktale
	* Version				:	v1.0
	* date					:	2022/4/21
	* Description			:	
******************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "lwip/dns.h"

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"

#include "lib/oofatfs/ff.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

#include "py/objstr.h"
#include "py/objlist.h"

#include "py/mperrno.h" // used by mp_is_nonblocking_error
#include "py/nlr.h"
#include "py/gc.h"
#include "config.h"
#include "mod_wifllink.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "system_int.h"
#include "ledseq.h"
#include "anop.h"

static const char* TAG = "wifilink";

//==============================================================================================================
#define MAX_STA_CONN (1)
#define UDP_SERVER_BUFSIZE      128
#define EXAMPLE_ESP_WIFI_CHANNEL   13
#define EXAMPLE_MAX_STA_CONN       1

static char WIFI_SSID[32] = "pyDrone";
static char WIFI_PWD[32] = "12345678" ;
#define EXAMPLE_ESP_WIFI_SSID "pyDrone"
#define EXAMPLE_ESP_WIFI_PASS  "12345678"
static uint32_t udp_server_port = 2390;
static struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6


static char rx_buffer[UDP_SERVER_BUFSIZE] = {0};
static char tx_buffer[UDP_SERVER_BUFSIZE] = {0};
const int addr_family = (int)AF_INET;
const int ip_protocol = IPPROTO_IP;
static struct sockaddr_in dest_addr;
static int sock;
static UDPPacket inPacket;
static UDPPacket outPacket;

static xQueueHandle udpDataRx;
static xQueueHandle udpDataTx;

#define DEBUG_UDP
//===================================================================================================================
typedef struct _wifilink_obj_t {
  mp_obj_base_t base;
} wifilink_obj_t;

static bool isInit = false;
static bool isUDPInit = false;
static bool isUDPConnected = false;
esp_netif_t *ap_netif = NULL;
//====================================================================================================================

STATIC mp_obj_t wifilink_deinit(mp_obj_t self_in) {

  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(wifilink_deinit_obj, wifilink_deinit);

bool wifiGetDataBlocking(UDPPacket *in)
{
	if(!isInit) return true;
    while (xQueueReceive(udpDataRx, in, portMAX_DELAY) != pdTRUE) {
        vTaskDelay(1);
    };
	
    return true;
};

bool getUDPConnectedStatus(void)
{
	return isUDPConnected;
}
bool wifiSendData(uint32_t size, uint8_t *data)
{
    static UDPPacket outStage;
	if(!isInit) return false;
    outStage.size = size;
    memcpy(outStage.data, data, size);

	if(xQueueSend(udpDataTx, &outStage, M2T(100)) == pdTRUE)
	{
		return true;
	}
    return false;
};


static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
        ESP_LOGI(TAG,"station "MACSTR" join, AID=%d",
                          MAC2STR(event->mac), event->aid);
		ledseqRun(LINK_LED, seq_linkup);

    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
		isUDPConnected = false;
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
        ESP_LOGI(TAG,"station "MACSTR" leave, AID=%d",
                          MAC2STR(event->mac), event->aid);
		ledseqStop(LINK_LED, seq_linkup);
    } 
}

static esp_err_t udp_server_create(void *arg)
{ 
    if (isUDPInit){
        return ESP_OK;
    }
    
    struct sockaddr_in *pdest_addr = &dest_addr;
    pdest_addr->sin_addr.s_addr = htonl(INADDR_ANY);
    pdest_addr->sin_family = AF_INET;
    pdest_addr->sin_port = htons(udp_server_port);

    sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if (sock < 0) {
        ESP_LOGE(TAG,"Unable to create socket: errno %d", errno);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG,"Socket created");

    int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
        ESP_LOGE(TAG,"Socket unable to bind: errno %d", errno);
    }
    ESP_LOGI(TAG,"Socket bound, port %d", udp_server_port);

    isUDPInit = true;
    return ESP_OK;
}

static void udp_server_tx_task(void *pvParameters)
{
 
    while (1) {
        if(isUDPInit == false) {
            vTaskDelay(20);
            continue;
        }
        if ((xQueueReceive(udpDataTx, &outPacket, 5) == pdTRUE) && isUDPConnected) 
		{           
           memcpy(tx_buffer, outPacket.data, outPacket.size);
			
            int err = sendto(sock, tx_buffer, outPacket.size, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
            if (err < 0) {
                ESP_LOGE(TAG,"Error occurred during sending: errno %d", errno);
                continue;
            }
        }    
    }
}

static void udp_server_rx_task(void *pvParameters)
{
	uint8_t sum_check = 0;
	uint8_t add_check = 0;
	
    socklen_t socklen = sizeof(source_addr);
    
    while (1) {
        if(isUDPInit == false) {
            vTaskDelay(20);
            continue;
        }
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer), 0, (struct sockaddr *)&source_addr, &socklen);
        /* command step - receive  01 from Wi-Fi UDP */
		
        if (len < 0) {
            ESP_LOGE(TAG,"recvfrom failed: errno %d", errno);
            break;
        } else if(len > WIFI_RX_TX_PACKET_SIZE - 4) {
            ESP_LOGI(TAG,"Received data length = %d > 64", len);
        } else {
            //copy part of the UDP packet
            rx_buffer[len] = 0;// Null-terminate whatever we received and treat like a string...
			if(!isUDPConnected) isUDPConnected = true;
			
			if(rx_buffer[0] == UP_HEAD){
				sum_check = 0;
				add_check = 0;
				check_data(&sum_check, &add_check, (uint8_t *)&rx_buffer[0], len-2);
				
				if(sum_check == rx_buffer[len-2] && add_check == rx_buffer[len-1]){
					if (rx_buffer[3] == (len - 6)){
						inPacket.size = len;
						memcpy(inPacket.data, rx_buffer, len);
						xQueueSend(udpDataRx, &inPacket, M2T(2));
					}
				}
			}else if(rx_buffer[0] == 0x55 && rx_buffer[len-1] == 0xAA){
				setAnopPlatform(1);//
				inPacket.size = len;
				memcpy(inPacket.data, rx_buffer, len); 
				xQueueSend(udpDataRx, &inPacket, M2T(2));
			}
        }
    }
}

void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ap_netif = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}
void wifiInit(void )
{
    if (isInit) {
        return;
    }

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_softap();
	
    esp_netif_ip_info_t ip_info = {
        .ip.addr = ipaddr_addr("192.168.43.42"),
        .netmask.addr = ipaddr_addr("255.255.255.0"),
        .gw.addr      = ipaddr_addr("192.168.43.42"),
    };
    ESP_ERROR_CHECK(esp_netif_dhcps_stop(ap_netif));
    ESP_ERROR_CHECK(esp_netif_set_ip_info(ap_netif, &ip_info));
    ESP_ERROR_CHECK(esp_netif_dhcps_start(ap_netif));

    ESP_LOGI(TAG,"wifi_init_softap complete.SSID:%s password:%s", WIFI_SSID, WIFI_PWD);

    udpDataRx = xQueueCreate(5, sizeof(UDPPacket)); /* Buffer packets (max 64 bytes) */
    udpDataTx = xQueueCreate(5, sizeof(UDPPacket)); /* Buffer packets (max 64 bytes) */
    if (udp_server_create(NULL) == ESP_FAIL) {
        ESP_LOGE(TAG,"UDP server create socket failed!!!");
    } else {
        ESP_LOGI(TAG,"UDP server create socket succeed!!!");
    } 
    xTaskCreate(udp_server_tx_task, UDP_TX_TASK_NAME, 4*1024, NULL, UDP_TX_TASK_PRI, NULL);
    xTaskCreate(udp_server_rx_task, UDP_RX_TASK_NAME, 4*1024, NULL, UDP_RX_TASK_PRI, NULL);
    isInit = true;
}

//----------------------------------------------------------------------------------
STATIC mp_obj_t drone_wifilink_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

	enum {ARG_ssid, ARG_pwd, ARG_port };
	STATIC const mp_arg_t allowed_args[] = {
		{ MP_QSTR_ssid,		MP_ARG_KW_ONLY | MP_ARG_OBJ,	{.u_obj = MP_OBJ_NULL}	},
		{ MP_QSTR_pwd,		MP_ARG_KW_ONLY | MP_ARG_OBJ,	{.u_obj = MP_OBJ_NULL}	},
		{ MP_QSTR_port,		MP_ARG_INT,						{.u_int = 2390}			},
	};

	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	if(args[ARG_ssid].u_obj !=MP_OBJ_NULL)
	{
		mp_buffer_info_t bufinfo;
		mp_get_buffer_raise(args[ARG_ssid].u_obj, &bufinfo, MP_BUFFER_READ);
		memset(WIFI_SSID,'\0',sizeof(WIFI_SSID));
		sprintf(WIFI_SSID,"%s",(char *)bufinfo.buf);
	}
	if(args[ARG_pwd].u_obj !=MP_OBJ_NULL)
	{
		mp_buffer_info_t bufinfo;
		mp_get_buffer_raise(args[ARG_pwd].u_obj, &bufinfo, MP_BUFFER_READ);
		memset(WIFI_PWD,'\0',sizeof(WIFI_PWD));
		sprintf(WIFI_PWD,"%s",(char *)bufinfo.buf);
	}

	udp_server_port = args[ARG_port].u_int;

	wifiInit();

	systemInit();

	anopInit();

	wifilink_obj_t *wifilink_obj;
	wifilink_obj = m_new_obj(wifilink_obj_t);
	wifilink_obj->base.type = &drone_wifilink_type;
	return MP_OBJ_FROM_PTR(wifilink_obj);
}
/******************************************************************************/
STATIC const mp_rom_map_elem_t drone_wifilink_locals_dict_table[] = {
	{ MP_ROM_QSTR(MP_QSTR__name__), MP_ROM_QSTR(MP_QSTR_wifi_udp) },
	{ MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&wifilink_deinit_obj) },
};
STATIC MP_DEFINE_CONST_DICT(drone_wifilink_locals_dict,drone_wifilink_locals_dict_table);

const mp_obj_type_t drone_wifilink_type = {
    { &mp_type_type },
    .name = MP_QSTR_wifi_udp,
    .make_new = drone_wifilink_make_new,
    .locals_dict = (mp_obj_dict_t *)&drone_wifilink_locals_dict,
};

