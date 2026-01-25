#include <info.h>
// ════════════════════════════════════════════════════════════════
// Verdrahtungshinweis
// ════════════════════════════════════════════════════════════════
void showConnect()
{
    Serial.println("");
    Serial.println("                  ┌──────────────────────┐");
    Serial.println("                  │   5V Netzteil        │");
    Serial.println("                  │   (3-5 Ampere)       │");
    Serial.println("                  └──────────┬───────────┘");
    Serial.println("                             │");
    Serial.println("                    ┌────────┴────────────┐");
    Serial.println("                    │                     │");
    Serial.println("                   5V                    GND");
    Serial.println("                    │                     │");
    Serial.println("      ┌─────────────┼─────────────────────┼──────────┐");
    Serial.println("      │             │                     │          │");
    Serial.println("      │  ┌──────────┴───┐         ┌───────┴──────┐   │");
    Serial.println("      │  │              │         │              │   │");
    Serial.println("  ┌───┴──┴─────┐    ┌───┴─────────┴────┐     ┌───┴───┴────┐");
    Serial.println("  │  Wemos     │    │   WS2812B Strip  │     │ Optional:  │");
    Serial.println("  │ D1 Mini    │    │   (118 LEDs)     │     │ 1000µF Cap │");
    Serial.println("  ├──────────  ┤    ├──────────────────┤     └────────────┘");
    Serial.println("  │            │    │                  │");
    Serial.println("  │ D6(GPIO12  ├────┤ DIN              │");
    Serial.println("  │            │    │ (evtl. via 470Ω) │");
    Serial.println("  │ 5V         ├───┬┤ 5V               │");
    Serial.println("  │            │   ││                  │");
    Serial.println("  │ GND        ├┬──│┤ GND              │");
    Serial.println("  │            ││  ││                  │");
    Serial.println("  │            ││  │└──────────────────┘");
    Serial.println("  │            ││  │┌────────────────────┐");
    Serial.println("  │            ││  ││ DS1307 RTC Modul   │");
    Serial.println("  │            ││  ││                    │");
    Serial.println("  │            ││  │├────────────────────┤");
    Serial.println("  │            ││  ││                    │");
    Serial.println("  │ D1 (GPIO5) ├│──│┤ SCL                │");
    Serial.println("  │            ││  ││                    │");
    Serial.println("  │ D2 (GPIO4) ├│──│┤ SDA                │");
    Serial.println("  └────────────┘│  ││                    │");
    Serial.println("                │  └┤ VCC                │");
    Serial.println("                │   │                    │");
    Serial.println("                └───┤ GND                │");
    Serial.println("                    │                    │");
    Serial.println("                    └────────────────────┘");
}
