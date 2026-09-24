/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "persistent_key_store.h"

#include <aliro/user_device/interface.h>

/*
 * Thin adapter from Aliro::Interface::UserDevice::PersistentKey to this
 * application's own persistent_key_store. Every function here does exactly
 * one thing: call into AliroUd::PersistentKey::Store. No storage or PSA
 * key-ownership logic lives in this file; see persistent_key_store.cpp and
 * persistent_key_backend.h for that.
 */
namespace Aliro::Interface::UserDevice::PersistentKey {

AliroError Lookup(::Aliro::UserDevice::CredentialHandle handle,
		   const ::Aliro::UserDevice::ReaderGroupSubIdentifier &readerGroupSubIdentifier,
		   RecordHandle &outRecord, CryptoTypes::KeyId &outKeyId)
{
	return AliroUd::PersistentKey::Store::Lookup(handle, readerGroupSubIdentifier, outRecord, outKeyId);
}

AliroError Replace(::Aliro::UserDevice::CredentialHandle handle,
		    const ::Aliro::UserDevice::ReaderGroupSubIdentifier &readerGroupSubIdentifier,
		    CryptoTypes::KeyId keyId, RecordHandle &outRecord)
{
	return AliroUd::PersistentKey::Store::Replace(handle, readerGroupSubIdentifier, keyId, outRecord);
}

AliroError Delete(RecordHandle record)
{
	return AliroUd::PersistentKey::Store::Delete(record);
}

AliroError Reset()
{
	return AliroUd::PersistentKey::Store::Reset();
}

} // namespace Aliro::Interface::UserDevice::PersistentKey
