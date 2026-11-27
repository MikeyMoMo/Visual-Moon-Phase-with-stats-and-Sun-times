//*******
// Fill in the required fields, below, then rename this file to secret.h
// Fill in the required fields, below, then rename this file to secret.h
//*******
#if defined CONFIG_FOR_USER1  // Change as needed and update the #define at about line 37 in the main.
const char* chSSID      = "SSID";           // Your router SSID.
const char* chPassword  = "WAP_PW";         // Your router password.
const double latitude   = 0.00000;          // Your lat location.   + is North, - is South
const double longitude  = 0.00000;          // Your long location.  + is East,  - is West
#define apiKey "********************************"  // From and for use by calls to: ipgeolocation.io

#else                         // My WAP credentials and location.

const char* chSSID      = "SSID";           // Your router SSID.
const char* chPassword  = "WAP_PW";         // Your router password.
const double latitude   = 0.00000;          // Your lat location.   + is North, - is South
const double longitude  = 0.00000;          // Your long location.  + is East,  - is West
#define apiKey "********************************"  // From and for use by calls to: ipgeolocation.io
#endif
