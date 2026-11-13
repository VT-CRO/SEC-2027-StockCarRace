#pragma once

#include <Arduino.h>

// Pass an initialized Stream, such as App Lab's Monitor after Monitor.begin().
// Discard the bytes currently available in one read batch. Use this before
// accepting commands to clear input left over from startup or a prior prompt.
void discardMonitorInput(Stream &input);

// Print a prompt on the same stream and wait indefinitely for any input byte.
// Clears old input before prompting, then consumes the reply and any trailing
// line ending that arrives during the short wait after the first byte.
// Use this for a press-Enter gate when the contents of the reply are irrelevant.
void waitForMonitorMessage(Stream &input, const char *prompt);

// Read command text a little at a time so the caller can keep checking timers
// or running other work while waiting for input. Keep one reader across polls
// so it can retain a partial command. The initialized stream must outlive it.
class MonitorCommandReader {
public:
  explicit MonitorCommandReader(Stream &input) : input(input) {}

  enum class Result {
    Pending, // No complete command yet. Call poll() again later.
    Command, // A complete command is available through command().
    Invalid  // The command exceeded 31 bytes or included nonprintable input.
  };

  // Reads at most one command per call. A carriage return, newline, or 150 ms
  // gap after the last byte ends a command. Empty lines are ignored. Printable
  // ASCII, including spaces, is preserved for the caller to interpret.
  Result poll();

  // After poll() returns Command, read this null-terminated text before the
  // next poll() can overwrite it. Copy it if you need to keep it longer.
  const char *command() const { return buffer; }

private:
  Result finish();

  Stream &input;
  char buffer[32] = {};
  size_t length = 0;
  bool invalid = false;
  unsigned long lastByteMs = 0;
};
