/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

namespace DoorLock::GestureAccess {

/**
 * @brief Type for detection callback
 */
using GestureDetectedCallback = void (*)();

/**
 * @brief Initialize the gesture access feature.
 *
 * @param callback single callback fired on confirmed (debounced) detection
 * rising edge (not on every per-frame model result).
 *
 * @return 0 on success, negative errno otherwise.
 */
int Init(GestureDetectedCallback callback);

/**
 * @brief Enable or disable the capture & detection loop
 *
 * @param active true: enable, false: disable
 */
void SetDetectionActive(bool active);

} // namespace DoorLock::GestureAccess
