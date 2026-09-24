#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Aliro::Interface::UserDevice::PersistentKey contract: durable Kpersistent
# storage keyed by {CredentialHandle, ReaderGroupSubIdentifier}, backed by
# opaque PSA key IDs.
#
# persistent_key.cpp is a thin adapter to persistent_key_store.cpp (the
# record engine), which in turn calls persistent_key_backend.h for durable
# PSA key ownership. persistent_key_persistence_settings.cpp and
# persistent_key_backend_psa.cpp are the real (hardware-backed)
# persistence/key backends; host tests swap in application-owned in-memory
# fakes instead (see tests/functional/subsys/aliro_nfc_user_device/common/).
target_sources(app PRIVATE
  persistent_key.cpp
  persistent_key_store.cpp
  persistent_key_persistence_settings.cpp
  persistent_key_backend_psa.cpp
)
