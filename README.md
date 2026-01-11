# System Pulse

System Pulse is a C project for a microkernel-style Linux resource monitor. It splits IO and logic into two layers:

- Controller (IO shell): only reads file contents into memory.
- Pure Function (logic core): only parses strings and computes usage.

Initial focus: /proc filesystem parsing.
