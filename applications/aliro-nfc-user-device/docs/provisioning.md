# Field-based provisioning workflow

Every mutating change to a credential or its mailbox goes through the
`aliro-ud` development CLI's in-memory staging transaction: `begin-create`
or `begin-update` opens exactly one candidate, field commands set or clear
staged values, and `commit` is the only operation that changes persistent
state (`abort` discards it). A `commit`, `delete`, or `reset` is always
serialized through the lifecycle coordinator (`src/lifecycle`): any active
NFC session is terminated first, then the storage transaction runs, then
NFC service resumes.

Every command returns exactly one deterministic `OK ...`/`ERR ...` line.
None ever prints a private key, `Kpersistent`, or session-key byte. The
example hex values below (`AA` repeated, `00 11 22 ...`) are placeholders
for illustration only — never real key or credential material.

## 1. Connect

```bash
west flash          # or use an already-flashed DK
```

Open a serial terminal on the DK's virtual UART, 115200-8N1, then confirm
the device is up:

```
uart:~$ aliro-ud info
OK version=0.2.0-awp2+0 init=running session_active=0 activation_attempts=0 rejected_apdus=0
```

## 2. Stage a new Access Credential

```
uart:~$ aliro-ud credential begin-create
OK
uart:~$ aliro-ud credential set-key <64 hex chars: 32 big-endian P-256 scalar bytes>
OK
uart:~$ aliro-ud credential set-policy 3
OK
uart:~$ aliro-ud credential set-binding 0 <reader_group_identifier_hex> direct <reader_public_key_hex>
OK
uart:~$ aliro-ud credential set-mailbox 8 3
OK
```

- `set-key` validates the scalar and imports it into PSA/CRACEN/KMU-backed
  persistent storage before any candidate is retained; only an opaque PSA
  key identifier is ever kept in the record.
- `set-policy` stages the `authentication_policy` value (`0x01`/`0x02`/`0x03`).
- `set-binding <index> <reader_group_identifier> direct|issuer <key_hex>`
  stages one `{ reader_group_identifier, trust_type, key }` binding; up to
  `CONFIG_ALIRO_UD_MAX_BINDINGS_PER_CREDENTIAL` (default 16) may be staged
  per candidate, each with an independently chosen key.
  `<reader_group_identifier>` is exactly 32 hex chars (16 bytes,
  `Aliro::UserDevice::kReaderGroupIdentifierLength`); `<key_hex>` is
  exactly 130 hex chars (65 bytes: a `0x04` uncompressed-point prefix plus
  two 32-byte P-256 coordinates, `Aliro::CryptoTypes::kEccP256PublicKeyLength`).
  A wrong-length value returns `ERR INVALID_ARGUMENT` for either field.
- `set-mailbox <size_bytes> <rights>` stages the mailbox size and a 2-bit
  rights mask (`bit0`=readable, `bit1`=writable) — `3` is read-write.
- Optional: `set-mailbox-data-subset <index> <offset> <length>`,
  `set-credential-timestamp <hex>`, `set-revocation-timestamp <hex>`,
  `set-access-document <hex>`, `set-revocation-document <hex>`.

Commit the staged candidate:

```
uart:~$ aliro-ud credential commit
OK handle=1
```

`commit` validates the complete candidate (`ValidatePayloadShape()`) before
any persistent state changes; a malformed candidate is rejected with an
`ERR ...` line and leaves previously committed state untouched. `abort`
discards the staged candidate without touching persistent state.

## 3. Inspect and manage

```
uart:~$ aliro-ud credential inspect 1
OK handle=1 bindings=1 policy=3 has_trust=1 has_mailbox=1 has_credential_timestamp=0 has_revocation_timestamp=0
uart:~$ aliro-ud credential list
OK handle=1 bindings=1 policy=3 has_trust=1 has_mailbox=1 has_credential_timestamp=0 has_revocation_timestamp=0
OK count=1
uart:~$ aliro-ud credential bindings 1
OK count=1 bindings=<reader_group_identifier_hex>
```

Update an existing credential (clones its non-secret state, retaining the
existing opaque key reference, unless `set-key` stages a replacement):

```
uart:~$ aliro-ud credential begin-update 1
OK
uart:~$ aliro-ud credential set-policy 2
OK
uart:~$ aliro-ud credential commit
OK handle=1
```

Select which credential is preferred for a given reader group:

```
uart:~$ aliro-ud credential preferred-set <reader_group_identifier_hex> 1
OK
uart:~$ aliro-ud credential preferred-get <reader_group_identifier_hex>
OK handle=1
```

`preferred-get` for a reader group with no preference set returns
`ERR NOT_SET command=credential preferred-get`.

## 4. Provision and inspect the mailbox

```
uart:~$ aliro-ud mailbox inspect 1
OK handle=1 size=8 readable=1 writable=1 data_subset_configured=0 data_subset_pairs=0 initialized=0 has_data=0
uart:~$ aliro-ud mailbox init 1
OK
uart:~$ aliro-ud mailbox inspect 1
OK handle=1 size=8 readable=1 writable=1 data_subset_configured=0 data_subset_pairs=0 initialized=1 has_data=0
uart:~$ aliro-ud mailbox read 1 0 8
OK data=0000000000000000
uart:~$ aliro-ud mailbox reset 1
OK
```

`mailbox inspect`/`read`/`init`/`reset` are Credential-Issuer-level
commands (they bypass Reader `MailboxPermissions`, per Aliro specification
§8.3.1.15) — the only path to committed mailbox bytes without a real
Reader-driven `EXCHANGE`.

## 5. Delete and factory-reset

```
uart:~$ aliro-ud credential delete 1
OK
uart:~$ aliro-ud credential list
OK count=0
uart:~$ aliro-ud mailbox inspect 1
ERR 4 command=mailbox inspect
```

`credential delete` erases the credential's mailbox as well (its
`MailboxHandle` is freed for reuse), confirmed above by `mailbox inspect`
now reporting an error for the deleted handle. `aliro-ud credential reset`
performs the same erasure for every provisioned credential.

## 6. Local button authorization

For `authentication_policy` values `0x01`–`0x03`, a valid window from a
real press of DK `Button 0` is required before the stack will continue an
AUTH0 exchange:

```
uart:~$ aliro-ud auth status
OK state=required remaining_ms=0
                                          # (press Button 0 here)
uart:~$ aliro-ud auth status
OK state=authorized remaining_ms=25983
```

`aliro-ud auth press`/`clear`/`notify-required` are host- and DK-test
triggers only (no NFC reader is required to exercise the window/indicator
logic); they behave identically to a real button press/expiry/stack-driven
`NotifyAuthenticationRequired()` call.

## 7. Persistence across reset and power loss

Every committed credential and mailbox record (Zephyr settings/NVS plus
PSA/CRACEN/KMU key material) survives a board reset or full power cycle
until an explicit `delete`/`reset`. This has been confirmed on a physical
DK: a real `RESET_SYSTEM` reset left `credential list`/`mailbox inspect`
reporting the same committed state as before the reset (see
`evidence/AWP6.md` and `evidence/AWP8.md`). General credential-field
power-loss fault injection beyond the mailbox portion remains a host-test
gap; see `traceability.md`'s `ALIRO-UD-SYRS-P1-007` row.

## Diagnostics

```
uart:~$ aliro-ud timing stats
OK enabled=1 samples=0 last_ms=0 max_ms=0
uart:~$ aliro-ud timing reset
OK
```

`timing stats`/`reset` report command-to-response duration statistics
(`CONFIG_ALIRO_UD_TIMING_INSTRUMENTATION`, default enabled); see
`architecture.md`'s "Timing and resource instrumentation" section.

## End-to-end NFC result and Expedited-Fast fallback

On 2026-09-04, the following two DKs were provisioned and tested:

- User Device: serial `1051885995`
- Reader: serial `1051889440`
- Shared reader group identifier: `4ea8ee0ee18e074fda76b14a59b38999`

The first NFC tap completed the Expedited-Standard path successfully:

- User Device logged `AUTH0 accepted` and `AUTH1 accepted, secure channel established`.
- Reader verified the signature, logged `ACCESS GRANTED`, sent `EXCHANGE`, and
  completed the simulated lock unlock.

The User Device subsequently reported the transaction as aborted after the
NFC field was removed; the Reader had already granted access and completed
the unlock.

On later taps, the Reader attempted Expedited-Fast instead. This User Device
application does not implement Expedited-Fast (it is explicitly excluded in
`APP_PLAN.md` §5). The User Device rejected the Fast `AUTH0` with decode
result `8`; the Reader received status `0x6A80` and did not continue with
Expedited-Standard. No access was granted in those attempts.

Aliro 1.0 specifies that a Reader can continue with Expedited-Standard after
an Expedited-Fast trial fails, while also allowing the Reader to abort for
security reasons (Aliro 1.0 Specification and Test Plan, §8.3.3.2.8,
pages 70–71). Therefore, the demonstrated result should be recorded as:

> Expedited-Standard works end-to-end. Automatic Expedited-Fast-to-Standard
> fallback was not observed with this current Reader/User Device firmware.

For a repeatable Expedited-Standard demonstration, disable
`CONFIG_DOOR_LOCK_EXPEDITED_FAST_PHASE` in the Reader build, or clear the
Reader's Kpersistent entries if the firmware provides that command. Press
User Device Button 0 immediately before each tap because policy `3` requires
a fresh authorization window.

See `traceability.md` and the per-AWP `evidence/AWP<n>.md` files for the
exact DK sessions this workflow's commands and output were captured from.
