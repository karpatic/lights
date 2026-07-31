# Archived Browser Upload Experiment - 2026-07-30

The old `upload.html` and `web.js` files are archived here because the browser upload flow was never compatible with the repository firmware.

The script attempted to send custom `CMD:*` messages over Web Serial, but the firmware does not implement that protocol and PlatformIO flashing still happens outside the browser. Do not restore this page as a current flashing path without designing and implementing matching firmware and a verified flashing/recovery flow.
