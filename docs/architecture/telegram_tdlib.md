# Architecture Decision: Telegram Provider (TDLib)

## Status
Accepted

## Context
FastHub requires a robust, native Telegram client implementation for Windows (x64/ARM64) that supports:
- Authentication (phone number, code, 2FA)
- Conversations and message history with pagination
- Sending and receiving messages, media, reactions
- Unread counters and notifications
- Local caching (LOCAL FIRST principle)
- Offline mode

## Decision
We choose **TDLib (Telegram Database Library)** as the underlying C++ library for the Telegram provider module (`providers/telegram`).

### Rationale:
1. **Official & Robust:** Developed by Telegram, TDLib handles the complex MTProto protocol, encryption, local SQLite storage, and synchronization out of the box.
2. **Native C++:** Aligns perfectly with FastHub's core technology stack (`C++20` + `Qt 6`).
3. **Performance & Security:** Highly optimized for desktop and mobile, supporting secure local storage and efficient incremental updates.

### Integration Approach:
- `ITelegramProvider` inherits from `core::IChatProvider`.
- All interactions with TDLib are encapsulated inside `providers/telegram`.
- Data flows through the Cache layer: `TDLib → TelegramProvider → SQLite Cache → UI`, respecting the LOCAL FIRST and FAST UI principles.
- Session credentials are securely stored using `ISecureStorage` (Windows DPAPI / Credential Manager).
