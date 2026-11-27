#if defined CONFIG_FOR_JOE    // My friend's WAP credentials and location
const char* chSSID      = "N_Port";           // Your router SSID.
const char* chPassword  = "helita1943";       // Your router password.
const double latitude   = 38.052147;          // Your lat location.   + is North, - is South
const double longitude  = -122.153893;        // Your long location.  + is East,  - is West
#define apiKey "********************************"  // For: ipgeolocation.io

#else                         // My WAP credentials and location.

//const char*  chSSID        = "Converge2G";       // Your router SSID.
//const char*  chPassword    = "Lallave@Family7";  // Your router password.
const char*  chSSID        = "MikeysWAP";          // Your router SSID.
const char*  chPassword    = "Noogly99";           // Your router password.
#define apiKey "8046078e6bc64b2784e7e6a51dc31343"  // For: ipgeolocation.io
const double latitude      = 18.5376;              // Your lat location.   + is North, - is South
const double longitude     = 120.7671;             // Your long location.  + is East,  - is West
#endif
