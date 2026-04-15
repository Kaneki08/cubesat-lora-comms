# LoRa Packet Structure & Implementation

## Packet Header (9 bytes)
```cpp
#pragma pack(push, 1)
struct PacketHeader {
  uint8_t sat_id;         // 1 byte
  uint8_t packet_type;    // 1 byte
  uint16_t sequence;      // 2 bytes
  uint32_t timestamp;     // 4 bytes
  uint8_t payload_len;    // 1 byte
};
#pragma pack(pop)
```

**Packet Types:**
- `PACKET_BATT = 1`
- `PACKET_IMU = 2`
- `PACKET_TC = 3`

## Payload Structures (with #pragma pack(1))

### IMU Payload (20 bytes)
```cpp
struct IMUPayload {
  int16_t gx_dps, gy_dps, gz_dps;  // Gyro
  int16_t ax_mg, ay_mg, az_mg;     // Accel
  int16_t qi, qj, qk, qr;          // Quaternion
};
```

### Thermocouple Payload (8 bytes)
```cpp
struct TCPayload {
  float tc_avg1;  // 4 bytes
  float tc_avg2;  // 4 bytes
};
```

### Battery Payload (TBD)
```cpp
struct BATTPayload {
  uint16_t voltage_mv;
  uint16_t current_ma;
  uint8_t soc_percent;
};
```

## C++ Transmitter

**encodeHex()** - Serializes header + payload into byte array using `memcpy()`

**transmit()** - Creates fresh header for each packet, encodes, and transmits via RadioLib

Keep structs in `HopeRFTX.h` with prototypes. Initialize struct data in `loop()` before each transmission to ensure fresh timestamps and sequence numbers.

## Example Reception (29 bytes - IMU with zeros)
```
Header (9 bytes):
0x01           = sat_id
0x02           = packet_type (PACKET_IMU)
0x00 0x00      = sequence
0xeb 0x18 0x00 0x00 = timestamp (6379 ms, little-endian)
0x14           = payload_len (20 decimal)

Payload (20 bytes):
0x00 0x00 ... 0x00 = All IMU fields = 0
```

## Python Receiver

Use **`PacketDecoder`** class with struct format strings:
- `HEADER_FORMAT = '<BBHIB'` → matches C++ PacketHeader
- `IMU_FORMAT = '<hhhhhhhhhh'` → 10 x int16_t
- `TC_FORMAT = '<ff'` → 2 x float

Decode bytes → extract header → identify payload type → parse payload accordingly.

Integrate into GNU Radio `lora_RX.grc` flowgraph for real-time decoding.

### PacketDecoder Implementation

```python
import struct

# Packet type constants
PACKET_IMU = 2
PACKET_TC = 3
PACKET_BATT = 1

class PacketDecoder:
    """Decodes LoRa packets matching C++ struct definitions"""
    
    # Struct format strings (matching #pragma pack(1))
    HEADER_FORMAT = '<BBHIB'  # sat_id, packet_type, sequence, timestamp, payload_len
    HEADER_SIZE = 9
    
    IMU_FORMAT = '<hhhhhhhhhh'  # 10 x int16_t
    IMU_SIZE = 20
    
    TC_FORMAT = '<ff'  # 2 x float
    TC_SIZE = 8
    
    def decode_header(self, raw_bytes):
        """Extract and parse packet header"""
        if len(raw_bytes) < self.HEADER_SIZE:
            raise ValueError(f"Packet too short: {len(raw_bytes)} < {self.HEADER_SIZE}")
        
        header_data = struct.unpack(self.HEADER_FORMAT, raw_bytes[:self.HEADER_SIZE])
        
        return {
            'sat_id': header_data[0],
            'packet_type': header_data[1],
            'sequence': header_data[2],
            'timestamp': header_data[3],
            'payload_len': header_data[4]
        }
    
    def decode_imu(self, raw_bytes):
        """Decode IMU payload"""
        if len(raw_bytes) < self.IMU_SIZE:
            raise ValueError(f"IMU payload too short: {len(raw_bytes)} < {self.IMU_SIZE}")
        
        imu_data = struct.unpack(self.IMU_FORMAT, raw_bytes[:self.IMU_SIZE])
        
        return {
            'gx_dps': imu_data[0],
            'gy_dps': imu_data[1],
            'gz_dps': imu_data[2],
            'ax_mg': imu_data[3],
            'ay_mg': imu_data[4],
            'az_mg': imu_data[5],
            'qi': imu_data[6],
            'qj': imu_data[7],
            'qk': imu_data[8],
            'qr': imu_data[9]
        }
    
    def decode_tc(self, raw_bytes):
        """Decode Thermocouple payload"""
        if len(raw_bytes) < self.TC_SIZE:
            raise ValueError(f"TC payload too short: {len(raw_bytes)} < {self.TC_SIZE}")
        
        tc_data = struct.unpack(self.TC_FORMAT, raw_bytes[:self.TC_SIZE])
        
        return {
            'tc_avg1': tc_data[0],
            'tc_avg2': tc_data[1]
        }
    
    def decode_packet(self, raw_bytes):
        """Decode complete packet: header + payload"""
        try:
            # Parse header
            header = self.decode_header(raw_bytes)
            
            # Extract payload portion
            payload_start = self.HEADER_SIZE
            payload_bytes = raw_bytes[payload_start:payload_start + header['payload_len']]
            
            # Decode payload based on packet type
            payload = None
            if header['packet_type'] == PACKET_IMU:
                payload = self.decode_imu(payload_bytes)
            elif header['packet_type'] == PACKET_TC:
                payload = self.decode_tc(payload_bytes)
            elif header['packet_type'] == PACKET_BATT:
                payload = {'error': 'BATT decoding not implemented'}
            else:
                payload = {'error': f'Unknown packet type: {header["packet_type"]}'}
            
            return {
                'header': header,
                'payload': payload,
                'raw_hex': ' '.join([f'0x{b:02X}' for b in raw_bytes])
            }
        
        except Exception as e:
            return {'error': str(e), 'raw_hex': ' '.join([f'0x{b:02X}' for b in raw_bytes])}
    
    def print_packet(self, decoded):
        """Pretty-print decoded packet"""
        if 'error' in decoded:
            print(f"❌ Decode Error: {decoded['error']}")
            print(f"Raw hex: {decoded['raw_hex']}")
            return
        
        header = decoded['header']
        payload = decoded['payload']
        
        print("\n" + "="*50)
        print("📡 LoRa Packet Decoded")
        print("="*50)
        
        print(f"\n--- Header ---")
        print(f"  SAT ID:       0x{header['sat_id']:02X}")
        print(f"  Packet Type:  {header['packet_type']} ", end="")
        
        if header['packet_type'] == PACKET_IMU:
            print("(IMU)")
        elif header['packet_type'] == PACKET_TC:
            print("(TC)")
        elif header['packet_type'] == PACKET_BATT:
            print("(BATT)")
        else:
            print("(UNKNOWN)")
        
        print(f"  Sequence:     {header['sequence']}")
        print(f"  Timestamp:    {header['timestamp']} ms")
        print(f"  Payload Len:  {header['payload_len']} bytes")
        
        print(f"\n--- Payload ---")
        if header['packet_type'] == PACKET_IMU:
            print(f"  gx_dps:  {payload['gx_dps']}")
            print(f"  gy_dps:  {payload['gy_dps']}")
            print(f"  gz_dps:  {payload['gz_dps']}")
            print(f"  ax_mg:   {payload['ax_mg']}")
            print(f"  ay_mg:   {payload['ay_mg']}")
            print(f"  az_mg:   {payload['az_mg']}")
            print(f"  qi:      {payload['qi']}")
            print(f"  qj:      {payload['qj']}")
            print(f"  qk:      {payload['qk']}")
            print(f"  qr:      {payload['qr']}")
        
        elif header['packet_type'] == PACKET_TC:
            print(f"  tc_avg1: {payload['tc_avg1']:.2f}°C")
            print(f"  tc_avg2: {payload['tc_avg2']:.2f}°C")
        
        print(f"\nRaw hex: {decoded['raw_hex']}")
        print("="*50 + "\n")
```

### GNU Radio Integration

```python
from lora_packet_decoder import PacketDecoder

class lora_RX(gr.top_block):
    def __init__(self):
        # ... existing code ...
        self.decoder = PacketDecoder()
    
    def process_received_packet(self, raw_bytes):
        """Called when a complete packet is received"""
        decoded = self.decoder.decode_packet(raw_bytes)
        self.decoder.print_packet(decoded)
        self.log_packet(decoded)
    
    def log_packet(self, decoded):
        """Save decoded packets to log file"""
        with open('/tmp/lora_packets.txt', 'a') as f:
            if 'error' not in decoded:
                header = decoded['header']
                f.write(f"[{header['timestamp']}ms] Type:{header['packet_type']} ")
                f.write(f"Seq:{header['sequence']} Payload:{decoded['payload']}\n")
```

## Important: `#pragma pack(1)`

Without it: compiler adds padding between struct members → larger packets
With it: no padding → exact size matching between C++ and Python

Critical for correct serialization with `memcpy()` and byte offset alignment.

## Next Steps

- [ ] Test all payload types (IMU, TC, BATT)
- [ ] Implement BATT decoder in Python
- [ ] Add overflow checks in `encodeHex()`
- [ ] Integrate `PacketDecoder` into GNU Radio
- [ ] Test long-range transmission
