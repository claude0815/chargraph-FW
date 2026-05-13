// ════════════════════════════════════════════════════════════════
//  RADIUS-Server-CA-Zertifikat fuer WPA2-Enterprise (PEAP)
// ════════════════════════════════════════════════════════════════
//
// Wird vom Connect-Code automatisch verwendet, wenn der PEM-Inhalt
// zwischen den R"PEM(...)PEM"-Delimitern unten nicht leer ist.
//
// Format
// ------
//  * PEM (kein DER): "-----BEGIN CERTIFICATE----- ... -----END CERTIFICATE-----"
//  * Bei einer Cert-Chain einfach mehrere PEM-Bloecke hintereinander einfuegen
//    (zuerst Leaf-/Server-Cert, dann Intermediate(s), zuletzt Root-CA).
//  * Maximale Groesse: ~3 KB Gesamt (sonst SDK-EAP-Fragmentation-Probleme).
//
// So beschaffst du das Cert
// -------------------------
//  Wireshark-Methode (universell, ~5 min):
//   1. WLAN trennen, Wireshark auf WLAN-Interface starten.
//   2. Display-Filter: tls.handshake.type == 11
//   3. WLAN neu verbinden -> EAP-Handshake laeuft.
//   4. Im Capture den TLS-Frame "Certificate" finden, in Tree aufklappen,
//      Rechtsklick auf "Certificate" -> "Export Packet Bytes..." -> .der.
//   5. openssl x509 -inform DER -in server.der -out server.pem
//
//  Linux/NetworkManager:
//   sudo grep ca-cert /etc/NetworkManager/system-connections/<SSID>.nmconnection
//
// Aktivierung / Deaktivierung
// ---------------------------
// Aktivierung: PEM zwischen den R"PEM(...)PEM"-Markern einfuegen.
// Deaktivierung: alles dazwischen loeschen (leerer String).
//
// Solange das Feld leer ist, wird kein CA-Cert gesetzt und der RADIUS-
// Server bleibt unverifiziert (= Verhalten bisher).

static const char wpa2_ca_cert[] = R"PEM()PEM";
