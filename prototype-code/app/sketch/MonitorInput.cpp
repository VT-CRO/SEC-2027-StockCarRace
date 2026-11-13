#include "MonitorInput.hpp"

namespace {
constexpr unsigned long COMMAND_GAP_MS = 150;
} // namespace

void discardMonitorInput(Stream &input) {
  for (int remaining = input.available(); remaining > 0; --remaining) {
    input.read();
  }
}

void waitForMonitorMessage(Stream &input, const char *prompt) {
  // Give leftover bytes from a previous interaction time to arrive.
  delay(300);
  discardMonitorInput(input);
  input.println(prompt);
  while (input.available() <= 0) {
    delay(10);
  }
  // The reply's first byte can arrive ahead of its line ending.
  delay(100);
  discardMonitorInput(input);
}

MonitorCommandReader::Result MonitorCommandReader::poll() {
  // RouterBridge fetches bytes from the router on available(). Read the
  // buffered batch before asking it for more.
  for (int remaining = input.available(); remaining > 0; --remaining) {
    const int byte = input.read();
    if (byte < 0) {
      break;
    }
    if (byte == '\r' || byte == '\n') {
      if (length > 0 || invalid) {
        return finish();
      }
      continue;
    }
    lastByteMs = millis();
    if (length < sizeof(buffer) - 1 && byte >= ' ' && byte <= '~') {
      buffer[length++] = static_cast<char>(byte);
    } else {
      invalid = true;
    }
  }

  if ((length > 0 || invalid) && millis() - lastByteMs >= COMMAND_GAP_MS) {
    // Accept messages sent by a Monitor configured without a line ending.
    return finish();
  }
  return Result::Pending;
}

MonitorCommandReader::Result MonitorCommandReader::finish() {
  buffer[length] = '\0';
  const Result result = invalid ? Result::Invalid : Result::Command;
  length = 0;
  invalid = false;
  return result;
}
