# ReadAll

[Русский](README.ru.md)

Select `src_dir = examples/ReadAll` in the repository's `platformio.ini`, choose
your board environment and open the serial monitor at 115200 baud. Set the
OpenTherm adapter GPIOs with `MASTER_RX_GPIO` and `MASTER_TX_GPIO`.
The included ESP32-S3 environment uses RX=35/TX=36; the sketch defaults are 4/5.

The example reads all 87 known IDs marked R or RW in the OpenTherm v4.2.1
overview map. Each `loop()` sends one synchronous read with up to ten total
attempts, including the first. `INVALID` results, timeouts and recoverable errors
are retried; a successful response or a valid negative reply ends the operation
early.
After the final result is printed, the example waits 150 ms before the next
request (ID or index).
After the last ID it waits 10 seconds before repeating the list.
Support depends on the connected device; a readable ID need not be implemented
by every boiler.

Every line contains the ID, name and operation status in aligned columns.
A complete response adds its raw frame and message type, including
`DATA_INVALID` or `UNKNOWN_DATA_ID` responses. These valid negative replies have
their own operation statuses; their payload is not decoded as a successful
reading. `INVALID` indicates a malformed or mismatched reply.
Successful results add the hexadecimal payload and decode it through
the matching message wrapper. Numeric IDs print their f8.8 or unsigned/signed
integer value. Structured IDs print every named field and flag exposed by their
wrapper: all Status flags, configuration fields, fault flags and codes, remote
parameter permissions, date/time components, indexed values and other fields.
ID30 uses the library's existing f8.8 interpretation.

TSP and fault-history count reads select how many indexed entries follow.
The example reads indices `0..count-1`, one per `loop()`, for the boiler,
ventilation/heat-recovery and solar-storage groups. A zero count skips that
group's indexed reads. If the count cannot be read, index 0 is probed once.
An error for one entry does not stop the remaining indices. `requestIndex`
is printed for every indexed result; successful replies also show the echoed
`index` and `value` through `IndexedValue`.

Indexed requests carry the index in HB and zero in LB. Other requests use
payload zero; brand strings still read character 0. Status reads send zero master
flags, including CH and DHW enable; use this example on a test setup where these
flags are appropriate. It does not send write requests.
