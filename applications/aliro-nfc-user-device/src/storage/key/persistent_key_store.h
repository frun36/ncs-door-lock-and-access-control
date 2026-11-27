/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <aliro/errors.h>
#include <aliro/user_device/types.h>

/**
 * @brief `Kpersistent` persistent-key-record engine.
 *
 * Owns the fixed-capacity record table, indexed by the exact
 * `{CredentialHandle, ReaderGroupSubIdentifier}` pair (Aliro 1.0
 * Specification, section 6.2, page 28), and the durable ownership of each
 * record's underlying PSA key material (`persistent_key_backend.h`).
 * `persistent_key.cpp` is a thin adapter translating this API to the
 * `Aliro::Interface::UserDevice::PersistentKey` contract.
 */
namespace AliroUd::PersistentKey::Store {

/** @brief Initializes the persistence and key backends, loading every persisted record. */
AliroError Init();

/** @copydoc ::Aliro::Interface::UserDevice::PersistentKey::Lookup */
AliroError Lookup(::Aliro::UserDevice::CredentialHandle handle,
		   const ::Aliro::UserDevice::ReaderGroupSubIdentifier &readerGroupSubIdentifier,
		   ::Aliro::UserDevice::PersistentKeyHandle &outRecord, ::Aliro::CryptoTypes::KeyId &outKeyId);

/** @copydoc ::Aliro::Interface::UserDevice::PersistentKey::Replace */
AliroError Replace(::Aliro::UserDevice::CredentialHandle handle,
		    const ::Aliro::UserDevice::ReaderGroupSubIdentifier &readerGroupSubIdentifier,
		    ::Aliro::CryptoTypes::KeyId keyId, ::Aliro::UserDevice::PersistentKeyHandle &outRecord);

/** @copydoc ::Aliro::Interface::UserDevice::PersistentKey::Delete */
AliroError Delete(::Aliro::UserDevice::PersistentKeyHandle record);

/** @copydoc ::Aliro::Interface::UserDevice::PersistentKey::Reset */
AliroError Reset();

/** @brief Deletes every persistent-key record for one credential (used by Credential::Store::Delete/Reset). */
AliroError DeleteAllForCredential(::Aliro::UserDevice::CredentialHandle handle);

} // namespace AliroUd::PersistentKey::Store
