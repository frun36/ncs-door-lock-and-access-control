#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Aliro::Interface::UserDevice::Document contract: immutable,
# snapshot-based access to a credential's provisioned Access/Revocation
# Documents (APP_PLAN.md AWP5).
#
# document.cpp is a thin adapter to document_snapshots.cpp (the open-
# snapshot engine) and storage/credential/credential_store.cpp (the single
# source of truth for provisioned document bytes). No separate persistence
# backend is needed: documents already live inside
# storage/credential/credential_persistence.h.
target_sources(app PRIVATE
  document.cpp
  document_snapshots.cpp
)
