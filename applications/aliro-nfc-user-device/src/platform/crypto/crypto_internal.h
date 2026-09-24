/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <aliro/errors.h>
#include <aliro/types.h>

#include <psa/crypto.h>

/**
 * @brief Narrow, non-public accessors into `crypto.cpp`'s imported-key
 * marker-handle scheme, exposed only to `storage/key/persistent_key_backend_psa.cpp`.
 *
 * `Aliro::Interface::UserDevice::Crypto::ImportKey()` returns an opaque
 * marker `KeyId` (never a raw PSA key handle) referencing a pair of
 * *volatile*, non-exportable PSA keys held in `crypto.cpp`'s own
 * `gImportedKeySlots` table. `PersistentKey::Replace()` needs to durably own
 * the derive-capable half of that key (via `psa_copy_key()`) before its
 * caller destroys the marker handle, and `PersistentKey::Lookup()` needs to
 * hand back a fresh, throwaway marker handle wrapping a volatile copy of a
 * durably-stored key. Neither operation belongs in the public
 * `Aliro::Interface::UserDevice::Crypto` contract, so they live here
 * instead.
 */
namespace AliroUd::Crypto::Internal {

/**
 * @brief Resolves an application-facing `KeyId` (marker or raw) to the
 * underlying derive-capable PSA key id.
 *
 * @return The resolved PSA key id, or `0` if `keyId` does not resolve to a
 * live derive-capable key.
 */
psa_key_id_t ResolveDeriveKeyIdForPersistence(::Aliro::CryptoTypes::KeyId keyId);

/**
 * @brief Registers an externally-created, derive-only PSA key id (e.g. a
 * fresh `psa_copy_key()` output) as a new imported-key marker slot, so it
 * can later be destroyed via the ordinary
 * `Aliro::Interface::UserDevice::Crypto::DestroyKey()` path.
 *
 * The registered slot has no AEAD half; `ResolveAeadKeyId()` on the
 * returned marker resolves to `0`.
 *
 * @param derivePsaKeyId A live, derive-capable PSA key id this module takes
 * ownership of.
 * @param outMarkerKeyId The new marker `KeyId` on success.
 *
 * @return `ALIRO_NO_ERROR` on success, error code otherwise (including "no
 * free slot"), leaving `derivePsaKeyId` untouched (still owned by the
 * caller) on failure.
 */
AliroError RegisterDeriveOnlyKey(psa_key_id_t derivePsaKeyId, ::Aliro::CryptoTypes::KeyId &outMarkerKeyId);

} // namespace AliroUd::Crypto::Internal
