import os
import gzip

html_path = "html/index.html"
header_path = "include/html.h"

# HTML-Datei lesen
with open(html_path, 'r', encoding='utf-8') as f:
    html_content = f.read()

# HTML-Inhalt mit GZIP komprimieren
html_bytes = html_content.encode('utf-8')
compressed = gzip.compress(html_bytes, compresslevel=9)

# Original- und komprimierte Größe ausgeben
original_size = len(html_bytes)
compressed_size = len(compressed)
ratio = (1 - compressed_size / original_size) * 100

print(f"Original-Größe:     {original_size:6d} Bytes")
print(f"Komprimierte Größe: {compressed_size:6d} Bytes")
print(f"Kompression:        {ratio:5.1f}%")

# Header-Datei erstellen mit komprimierten Daten
header_content = f"""// Auto-generiert von html_to_header.py
// Original-Größe:     {original_size} Bytes
// Komprimierte Größe: {compressed_size} Bytes
// Kompression:        {ratio:.1f}%

#ifndef HTML_H
#define HTML_H

#include <Arduino.h>

// GZIP-komprimierte HTML-Seite
const uint8_t HTML_PAGE_GZIP[] PROGMEM = {{
"""

# Komprimierte Bytes als Hex-Array schreiben
for i, byte in enumerate(compressed):
    if i % 16 == 0:
        header_content += "    "
    header_content += f"0x{byte:02x}"
    if i < len(compressed) - 1:
        header_content += ","
    if (i + 1) % 16 == 0:
        header_content += "\n"
    else:
        header_content += " "

header_content += f"""
}};

const size_t HTML_PAGE_GZIP_LEN = {compressed_size};

#endif // HTML_H
"""

# Header-Datei schreiben
os.makedirs(os.path.dirname(header_path), exist_ok=True)
with open(header_path, 'w', encoding='utf-8') as f:
    f.write(header_content)

print(f"\nHeader-Datei erstellt: {header_path}")
