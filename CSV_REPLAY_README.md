# CSV to CAN Replay System

## Overview

This tool sends CAN messages from ENGINE_DATA rows in a CSV file over a CAN bus. It reads time-series engine data (Torque and RPM) from a CSV file and replays them as CAN frames in real-time, following the DBC signal definitions.

## Quick Start

```bash
cd KVASER_CAN_
.\can_sender.exe                          # Use default CSV file
.\can_sender.exe test_data.csv            # Use custom CSV file
.\can_sender.exe -v engine_test_data.csv  # Verbose output
.\can_sender.exe --help                   # Display help
```

## Features

- **CSV-driven playback**: Load engine data from CSV files and replay to CAN bus
- **Real-time timing**: Messages are sent with accurate timing based on CSV time column
- **DBC-compliant encoding**: Signals are encoded according to DBC specifications
- **Verbose logging**: Optional detailed output for debugging
- **Flexible file resolution**: Searches current directory, executable directory, and workspace-relative paths

## CSV File Format

The CSV file must have the following structure:

### Required Columns

| Column Name | Unit | Range | Description |
|------------|------|-------|-------------|
| `Time` | seconds | [0, ∞) | Elapsed time (non-negative, monotonically increasing) |
| `ENGINE_Torque` | Nm (Newton-meters) | [0, 6553.5] | Engine torque value |
| `ENGINE_RPM` | rpm (revolutions per minute) | [0, 16383.75] | Engine speed |

### Example CSV (engine_test_profile.csv)

```csv
Time,ENGINE_Torque,ENGINE_RPM
0.00,250.0,900.0
0.01,252.5,925.3
0.02,255.0,950.6
...
10.00,8.0,922.75
```

### CSV Validation Rules

- **Time column**:
  - Must be non-negative
  - Must be monotonically increasing (each row > previous row)
  - Values start at 0.0 seconds

- **ENGINE_Torque column**:
  - Must be in range [0, 6553.5] Nm
  - Clamped to DBC signal range during encoding

- **ENGINE_RPM column**:
  - Must be in range [0, 16383.75] rpm
  - Clamped to DBC signal range during encoding

## CAN Message Details

### Message: ENGINE_DATA

| Attribute | Value |
|-----------|-------|
| CAN ID | 0x7B (123 decimal) |
| Frame Type | Standard frame (11-bit ID) |
| DLC (Data Length Code) | 8 bytes |

### Signal Layout

**Bytes 0-1: ENGINE_Torque (Little-Endian)**
- Scale: 0.1 Nm/unit
- Offset: 0 Nm
- Data type: unsigned 16-bit integer
- Formula: `raw_value = torque_nm / 0.1`
- Max value: 6553.5 Nm (65535 units)

**Bytes 2-3: ENGINE_RPM (Little-Endian)**
- Scale: 0.25 rpm/unit
- Offset: 0 rpm
- Data type: unsigned 16-bit integer
- Formula: `raw_value = rpm / 0.25`
- Max value: 16383.75 rpm (65535 units)

**Bytes 4-7: Reserved**
- Currently unused (set to 0x00)

### HEX Frame Example

For ENGINE_Torque=320 Nm and ENGINE_RPM=900 rpm:

```
Raw values:
  ENGINE_Torque: 320 / 0.1 = 3200 (0x0C80 in hex)
  ENGINE_RPM: 900 / 0.25 = 3600 (0x0E10 in hex)

CAN Frame (little-endian):
  Bytes 0-1: 0x80 0x0C  (torque, little-endian)
  Bytes 2-3: 0x10 0x0E  (rpm, little-endian)
  Bytes 4-7: 0x00 0x00 0x00 0x00  (reserved)
```

## Usage Examples

### Basic Usage: Default CSV File

```bash
.\can_sender.exe
```

Loads `engine_test_profile.csv` from the current/exe/workspace directory and sends all samples to CAN bus.

### Custom CSV File

```bash
.\can_sender.exe my_engine_data.csv
```

Loads `my_engine_data.csv` and replays it.

### Verbose Output

```bash
.\can_sender.exe -v engine_data.csv
```

Enables verbose logging to show:
- CSV loading status
- Sample count
- Playback completion message

### Display Help

```bash
.\can_sender.exe --help
```

Shows usage information and format requirements.

## Playback Behavior

1. **Initialization**: Opens CAN channel, configures bus to 500 kbps, starts CAN bus
2. **Timing**: Messages are sent sequentially with delays based on CSV time column
   - Delay between sample N and N+1 = `time[N+1] - time[N]` (in seconds, converted to milliseconds)
3. **Encoding**: Each sample is encoded into an 8-byte CAN frame according to DBC specifications
4. **Transmission**: Frames are sent via `canWrite()` to the CAN bus
5. **Graceful Shutdown**: CAN bus is stopped, channel closed, resources released

## Error Messages

### CSV Format Errors

```
✗ CSV load failed: CSV header mismatch. Expected: Time, ENGINE_Torque, ENGINE_RPM. Found: Time, Torque, RPM
```

**Solution**: Verify column names in CSV file header match exactly: `Time`, `ENGINE_Torque`, `ENGINE_RPM`

### Range Validation Errors

```
✗ CSV load failed: CSV validation error at row 42: ENGINE_Torque 6600.5 Nm is outside DBC range [0, 6553.5]
```

**Solution**: Check row 42 (42 - 1 row for header) and ensure values are within valid ranges.

### File Not Found Errors

```
✗ CSV file not found: nonexistent.csv
   Tried: nonexistent.csv, from executable dir, from workspace
```

**Solution**: Verify CSV file path is correct or copy file to KVASER_CAN_ directory.

## Building

Build using the provided task:

```bash
# In VS Code terminal
-v Build can_sender
```

Or use g++ directly:

```bash
g++ -o can_sender.exe can_sender.cpp csv_parser.cpp ^
  -IC:/Program Files (x86)/Kvaser/Canlib/Inc ^
  -LC:/Program Files (x86)/Kvaser/Canlib/Lib/MS ^
  -l:canlib32.lib -l:kvaDbLib.lib ^
  -std=c++11
```

## Testing

### Test 1: Default CSV Playback

```bash
.\can_sender.exe
```

Expected: All 1001 samples transmitted successfully

### Test 2: Verbose Mode

```bash
.\can_sender.exe -v > playback.log 2>&1
```

Expected: Log shows CSV load success, sample count, completion message

### Test 3: Help Display

```bash
.\can_sender.exe --help
```

Expected: Help text displayed with usage examples and CSV format requirements

### Test 4: Invalid CSV

```bash
.\can_sender.exe nonexistent.csv
```

Expected: Error message indicating file not found with search locations

## Dependencies

- **KVASER CANLIB**: 32-bit MS variant (canlib32.lib, kvaDbLib.lib)
- **rapidcsv**: Header-only CSV library (BSD-3-Clause license)
- **C++11 or later**: For compilation

## Files

| File | Purpose |
|------|---------|
| `can_sender.cpp/h` | Main CAN sender implementation and CLI |
| `csv_parser.cpp/h` | CSV parsing with validation |
| `engine_test_profile.csv` | Default test data (1001 samples, 0-10s) |
| `message_db.dbc` | DBC file with ENGINE_DATA signal definitions |
| `third_party/rapidcsv.h` | External CSV library (vendored) |

## Troubleshooting

### Issue: "CAN channel not available"

**Cause**: CAN interface not connected or in use

**Solution**:
1. Check that KVASER CAN Interface is connected
2. Verify no other application is using the CAN interface
3. Run as administrator if permission restricted

### Issue: "CSV load failed: Failed to read CSV file"

**Cause**: File format issues (encoding, line endings, etc.)

**Solution**:
1. Save CSV as UTF-8 with CRLF line endings
2. Verify columns are tab or comma-separated
3. Check for special characters in values

### Issue: Exit code 1 with unclear error

**Solution**: Use verbose mode to see detailed error messages:

```bash
.\can_sender.exe -v test_file.csv 2>&1
```

## Future Enhancements

- [ ] Support multiple CSV file playback modes (sequential, loop)
- [ ] Add message filtering (send every Nth message)
- [ ] JSON output format for programmatic consumption
- [ ] Support additional CAN signals beyond ENGINE_DATA
- [ ] Real-time message rate adjustment

## License

This tool uses the KVASER CANLIB library (proprietary) and rapidcsv (BSD-3-Clause).

## Support

For issues related to:
- **CSV format**: Check CSV_FORMAT section in this README
- **CAN communication**: See KVASER CANLIB documentation
- **Compilation**: Verify build paths in tasks.json match your system
