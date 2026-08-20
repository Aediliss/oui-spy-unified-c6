#ifndef MODES_H
#define MODES_H

// Mode 1: OUI Spy Detector
void detector_setup();
void detector_loop();
void detector_stop();

// Mode 2: Foxhunter
void foxhunter_setup();
void foxhunter_loop();
void foxhunter_stop();

// Mode 3: Flock-You — Promiscuous WiFi Edition
void flockyou_promiscious_setup();
void flockyou_promiscious_loop();
void flockyou_promiscious_stop();

// Mode 4: PCAP — Passive WiFi Packet Capture
void pcap_setup();
void pcap_loop();
void pcap_stop();

// Mode 5: Sky Spy
void skyspy_setup();
void skyspy_loop();
void skyspy_stop();

// Mode 6: BLE Sniff — Passive BLE advertising capture
void blesniff_setup();
void blesniff_loop();
void blesniff_stop();

#endif // MODES_H
