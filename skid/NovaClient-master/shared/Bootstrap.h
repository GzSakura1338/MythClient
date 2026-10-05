#pragma once

#include <stdint.h>

#define NOVA_BOOTSTRAP_MAGIC 0x4E564231u
#define NOVA_BOOTSTRAP_ABI 1u

enum NovaClientId {
#define NOVA_PROFILE(symbol, value, key, label, game) NovaClient_##symbol = value,
#include "ClientProfiles.inc"
#undef NOVA_PROFILE
};

enum NovaBootState {
    NovaBoot_Pending = 0,
    NovaBoot_Ready = 1,
    NovaBoot_Failed = 2
};

typedef struct NovaBootstrap {
    uint32_t magic;
    uint32_t size;
    uint32_t abi;
    uint32_t client;
    volatile int32_t state;
    char message[256];
} NovaBootstrap;
