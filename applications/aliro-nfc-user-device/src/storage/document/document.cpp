/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "document_snapshots.h"

#include "storage/credential/credential_store.h"

#include <aliro/user_device/interface.h>

/*
 * Thin adapter from Aliro::Interface::UserDevice::Document to
 * document_snapshots.cpp (the open-snapshot engine, Open/GetSize/Read/Close)
 * and storage/credential/credential_store.cpp (the single source of truth
 * for provisioned document bytes, Delete/Reset). No storage or snapshot
 * logic lives in this file.
 */
namespace Aliro::Interface::UserDevice::Document {

AliroError Open(::Aliro::UserDevice::CredentialHandle handle, ::Aliro::AccessDocumentTypes::DocumentType type,
		 SnapshotHandle &outSnapshot)
{
	return AliroUd::Document::Snapshots::Open(handle, type, outSnapshot);
}

AliroError GetSize(SnapshotHandle snapshot, size_t &outSize)
{
	return AliroUd::Document::Snapshots::GetSize(snapshot, outSize);
}

AliroError Read(SnapshotHandle snapshot, size_t offset, uint8_t *outData, size_t length)
{
	return AliroUd::Document::Snapshots::Read(snapshot, offset, outData, length);
}

void Close(SnapshotHandle snapshot)
{
	AliroUd::Document::Snapshots::Close(snapshot);
}

AliroError Delete(::Aliro::UserDevice::CredentialHandle handle, ::Aliro::AccessDocumentTypes::DocumentType type)
{
	return AliroUd::Credential::Store::DeleteDocument(handle, type);
}

AliroError Reset()
{
	AliroError firstError{ ALIRO_NO_ERROR };

	constexpr auto kMaxCredentials =
		static_cast<::Aliro::UserDevice::CredentialHandle>(AliroUd::Credential::kMaxCredentials);
	for (::Aliro::UserDevice::CredentialHandle handle = 1; handle <= kMaxCredentials; ++handle) {
		for (const auto type :
		     { ::Aliro::AccessDocumentTypes::DocumentType::Access,
		       ::Aliro::AccessDocumentTypes::DocumentType::Revocation }) {
			const auto error = AliroUd::Credential::Store::DeleteDocument(handle, type);
			/* ALIRO_INVALID_ARGUMENT here just means "no such credential"; not a Reset() failure. */
			if (error != ALIRO_NO_ERROR && error != ALIRO_INVALID_ARGUMENT &&
			    firstError == ALIRO_NO_ERROR) {
				firstError = error;
			}
		}
	}

	return firstError;
}

} // namespace Aliro::Interface::UserDevice::Document
