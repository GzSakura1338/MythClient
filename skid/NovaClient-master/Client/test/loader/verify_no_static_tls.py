"""Reject static TLS storage unsupported by Nova's reflective DLL loader."""
import struct
import sys
from pathlib import Path


def static_tls_size(path):
    data = Path(path).read_bytes()
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    if data[:2] != b'MZ' or data[pe:pe+4] != b'PE\0\0':
        raise ValueError('Not a PE image')
    optional = pe + 24
    if struct.unpack_from('<H', data, optional)[0] != 0x20b:
        raise ValueError('Expected a 64-bit DLL')
    rva, size = struct.unpack_from('<II', data, optional + 112 + 9*8)
    if rva == 0:
        return 0
    if size < 40:
        raise ValueError('Truncated TLS directory')
    section_table = optional + struct.unpack_from('<H', data, pe+20)[0]
    count = struct.unpack_from('<H', data, pe+6)[0]
    sections = [struct.unpack_from('<8sIIII', data, section_table+index*40) for index in range(count)]

    def offset(address, length):
        for _, _, section_address, raw_size, raw_offset in sections:
            relative = address-section_address
            if 0 <= relative and relative+length <= raw_size:
                return raw_offset+relative
        raise ValueError('TLS data outside file-backed sections')

    start, end, _, callbacks, zero_fill, _ = struct.unpack_from('<QQQQII', data, offset(rva, 40))
    image_base = struct.unpack_from('<Q', data, optional+24)[0]
    if callbacks and struct.unpack_from('<Q', data, offset(callbacks-image_base, 8))[0]:
        raise ValueError('TLS callbacks are unsupported by the reflective loader')
    # MSVC retains a single zero byte (_tls_start) and an empty callback table
    # even when there are no TLS variables. This sentinel has no consumers.
    if end-start == 1 and zero_fill == 0 and data[offset(start-image_base, 1)] == 0:
        return 0
    return end-start+zero_fill


if __name__ == '__main__':
    dll = sys.argv[1] if len(sys.argv) > 1 else Path(__file__).resolve().parents[2] / 'build-vs2026/Release/NovaClientBase.dll'
    size = static_tls_size(dll)
    if size:
        sys.exit(f'FAIL: {size} bytes of PE static TLS; unsafe with Nova reflective injection')
    print('PASS: DLL has no active PE static TLS storage or callbacks')
