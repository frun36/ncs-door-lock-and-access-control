/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "persistent_key_persistence.h"

#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include <cstdio>

LOG_MODULE_DECLARE(aliro_ud_key, CONFIG_ALIRO_UD_KEY_LOG_LEVEL);

/*
 * Real Zephyr settings-backed implementation, mirroring
 * storage/credential/credential_persistence_settings.cpp: bypasses the
 * settings_handler registration mechanism and queries the backend directly
 * by key, since this is a small, fixed set of well-known records.
 */
namespace AliroUd::PersistentKey::Persistence {
namespace {

int SlotKey(size_t slotIndex, char *buf, size_t bufLen)
{
	return snprintf(buf, bufLen, "aliro_ud/pkey/%zu", slotIndex);
}

} // namespace

AliroError Init()
{
	const int rc = settings_subsys_init();

	if (rc != 0) {
		LOG_ERR("settings_subsys_init() failed: %d", rc);
		return ALIRO_ERROR_INTERNAL;
	}

	return ALIRO_NO_ERROR;
}

AliroError LoadRecord(size_t slotIndex, Record &out, bool &outPresent)
{
	char key[24];
	SlotKey(slotIndex, key, sizeof(key));

	Record loaded{};
	const ssize_t rc = settings_load_one(key, &loaded, sizeof(loaded));

	if (rc == 0 || rc == -ENOENT) {
		outPresent = false;
		return ALIRO_NO_ERROR;
	}

	if (rc < 0) {
		LOG_ERR("settings_load_one(%s) failed: %zd", key, rc);
		return ALIRO_ERROR_INTERNAL;
	}

	if (static_cast<size_t>(rc) != sizeof(loaded)) {
		LOG_ERR("settings_load_one(%s) returned unexpected length %zd (expected %zu)", key, rc,
			sizeof(loaded));
		return ALIRO_ERROR_INTERNAL;
	}

	out = loaded;
	outPresent = true;
	return ALIRO_NO_ERROR;
}

AliroError SaveRecord(size_t slotIndex, const Record &value)
{
	char key[24];
	SlotKey(slotIndex, key, sizeof(key));

	const int rc = settings_save_one(key, &value, sizeof(value));
	if (rc != 0) {
		LOG_ERR("settings_save_one(%s) failed: %d", key, rc);
		return ALIRO_ERROR_INTERNAL;
	}

	return ALIRO_NO_ERROR;
}

AliroError EraseRecord(size_t slotIndex)
{
	char key[24];
	SlotKey(slotIndex, key, sizeof(key));

	const int rc = settings_delete(key);
	if (rc != 0) {
		LOG_ERR("settings_delete(%s) failed: %d", key, rc);
		return ALIRO_ERROR_INTERNAL;
	}

	return ALIRO_NO_ERROR;
}

} // namespace AliroUd::PersistentKey::Persistence
