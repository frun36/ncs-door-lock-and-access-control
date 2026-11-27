/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "persistent_key_types.h"

#include <aliro/errors.h>

/**
 * @brief Non-secret persistent storage for the `PersistentKey` record table.
 *
 * Stores only `{CredentialHandle, ReaderGroupSubIdentifier, persisted PSA
 * key id}` per record - never raw key bytes (the key material itself lives
 * in PSA persistent key storage, owned by `persistent_key_backend.h`).
 * Mirrors `storage/credential/credential_persistence.h`'s shape: a small,
 * purpose-built abstraction so the real implementation
 * (`persistent_key_persistence_settings.cpp`) can be a thin wrapper over
 * Zephyr settings/NVS-or-ZMS, and host tests can link an in-memory fake.
 *
 * Every function is synchronous and blocking; callers serialize concurrent
 * access themselves (the persistent-key store's own mutex).
 */
namespace AliroUd::PersistentKey::Persistence {

/** @brief Initializes the persistence backend (loads the settings subsystem on target). */
AliroError Init();

/** @brief Loads one record slot. */
AliroError LoadRecord(size_t slotIndex, Record &out, bool &outPresent);

/** @brief Persists one record slot, overwriting any previous value. */
AliroError SaveRecord(size_t slotIndex, const Record &value);

/** @brief Erases one record slot, if any. Idempotent. */
AliroError EraseRecord(size_t slotIndex);

} // namespace AliroUd::PersistentKey::Persistence
