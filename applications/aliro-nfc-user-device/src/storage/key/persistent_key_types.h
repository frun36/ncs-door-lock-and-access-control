/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <aliro/types.h>
#include <aliro/user_device/types.h>

#include <cstddef>

/**
 * @brief `PersistentKey` persistence data model.
 *
 * Plain-old-data so it can be persisted verbatim
 * (`persistent_key_persistence.h`) and constructed/compared directly in
 * tests. Never holds raw key bytes: `mPersistedKeyId` is only the opaque PSA
 * key id the actual `Kpersistent` material lives at (see
 * `persistent_key_backend.h`).
 */
namespace AliroUd::PersistentKey {

/** @brief Maximum number of persistent-key records, across all credentials. */
constexpr size_t kMaxRecords{ CONFIG_ALIRO_UD_MAX_CREDENTIALS *
			      CONFIG_ALIRO_UD_MAX_PERSISTENT_KEYS_PER_CREDENTIAL };

/** @brief Maximum number of persistent-key records per credential. */
constexpr size_t kMaxRecordsPerCredential{ CONFIG_ALIRO_UD_MAX_PERSISTENT_KEYS_PER_CREDENTIAL };

/** @brief One committed, persisted `Kpersistent` record. */
struct Record {
	bool mValid{ false };
	::Aliro::UserDevice::PersistentKeyHandle mHandle{ ::Aliro::UserDevice::kInvalidPersistentKeyHandle };
	::Aliro::UserDevice::CredentialHandle mCredentialHandle{ ::Aliro::UserDevice::kInvalidCredentialHandle };
	::Aliro::UserDevice::ReaderGroupSubIdentifier mReaderGroupSubIdentifier{};
	::Aliro::CryptoTypes::KeyId mPersistedKeyId{ 0 };
};

} // namespace AliroUd::PersistentKey
