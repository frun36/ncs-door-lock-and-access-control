/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "wifi.h"

#include <doorlock/utils/utils.h>

#include <net/wifi_ready.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>

LOG_MODULE_REGISTER(door_lock_wifi, CONFIG_DOOR_LOCK_WIFI_LOG_LEVEL);

namespace DoorLock::Wifi {

namespace {

constexpr uint64_t kWifiEvents{ NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT };
constexpr uint64_t kIpv4Events{ NET_EVENT_IPV4_ADDR_ADD };

net_mgmt_event_callback sWifiEventCb{};
net_mgmt_event_callback sIpv4EventCb{};

void HandleWifiEvent(net_mgmt_event_callback *cb, uint64_t event, net_if *)
{
	const auto *status = static_cast<const wifi_status *>(cb->info);

	switch (event) {
	case NET_EVENT_WIFI_CONNECT_RESULT:
		if (status->status != 0) {
			LOG_WRN("Wi-Fi connection failed: %d", status->status);
		} else {
			LOG_INF("Wi-Fi connected");
		}
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		LOG_INF("Wi-Fi disconnected: %d", status->status);
		break;
	default:
		break;
	}
}

void HandleIpv4Event(net_mgmt_event_callback *, uint64_t event, net_if *iface)
{
	VerifyOrReturn(event == NET_EVENT_IPV4_ADDR_ADD);

	const in_addr *addr = net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED);
	VerifyOrReturn(addr != nullptr);

	char addrStr[NET_IPV4_ADDR_LEN];
	LOG_INF("IPv4 address: %s", net_addr_ntop(AF_INET, addr, addrStr, sizeof(addrStr)));
}

void HandleWifiReady(bool ready)
{
	if (!ready) {
		LOG_WRN("Wi-Fi not ready");
		return;
	}

	LOG_INF("Wi-Fi ready");
}

} // namespace

int Init()
{
	net_if *iface = net_if_get_wifi_sta();
	VerifyOrReturnValue(iface != nullptr, -ENODEV, LOG_ERR("Wi-Fi interface not found"));

	net_mgmt_init_event_callback(&sWifiEventCb, HandleWifiEvent, kWifiEvents);
	net_mgmt_add_event_callback(&sWifiEventCb);

	net_mgmt_init_event_callback(&sIpv4EventCb, HandleIpv4Event, kIpv4Events);
	net_mgmt_add_event_callback(&sIpv4EventCb);

	wifi_ready_callback_t readyCb{};
	readyCb.wifi_ready_cb = HandleWifiReady;
	readyCb.iface = iface;

	int err = register_wifi_ready_callback(readyCb, iface);
	VerifyOrReturnValue(err == 0, err, LOG_ERR("Failed to register Wi-Fi ready callback: %d", err));

	return 0;
}

} // namespace DoorLock::Wifi
