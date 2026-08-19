/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

namespace DoorLock::GestureAccess {

using GestureDetectedCallback = void (*)();

int Init(GestureDetectedCallback callback);

void SetDetectionActive(bool active);

} // namespace DoorLock::GestureAccess
