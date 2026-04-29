// Shared author/copyright branding for all ZeddiHub TeamSpeak plugins.
// Inline-only; each plugin includes this header (relative path: ../../common/).

#pragma once

#define ZH_AUTHOR    "zeddis.xyz"
#define ZH_COPYRIGHT "Copyright (C) 2026 ZeddiHub.eu"
#define ZH_WEBSITE   "https://zeddihub.eu"

// Helper to compose a description string with copyright line.
//   "Foo. Bar. (c) 2026 ZeddiHub.eu | zeddis.xyz"
#define ZH_DESC(short_desc) short_desc " | " ZH_AUTHOR " | " ZH_COPYRIGHT
