/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

namespace DoorLock::Wifi {

/**
 * @brief Initializes the Wi-Fi station.
 *
 * Registers handlers that log the Wi-Fi readiness, connection state and IPv4 address. Connecting to the stored network
 * is handled by the connection manager.
 *
 * @return 0 on success, negative errno code otherwise.
 */
int Init();

} // namespace DoorLock::Wifi
