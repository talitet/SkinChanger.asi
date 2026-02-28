#include <Windows.h>
#include "plugin.h"
#include <iostream>
#include <RakHook/rakhook.hpp>
#include "RakNet/StringCompressor.h"
#include <sampapi/0.3.7-R3-1/CChat.h>
#include <sampapi/0.3.7-R3-1/CInput.h>
#include "RakNet/BitStream.h"
#include "RakNet/PacketEnumerations.h"
#include <string>
#include "detail.hpp"

using game_loop_t = void (*)();
using wndproc_t = LRESULT(CALLBACK*)(HWND, UINT, WPARAM, LPARAM);

static uint32_t skinId = 0;
static uint16_t playerId = 0;
static bool enabled = false;
static uint32_t realSkinId = 0;

static uint32_t lastSkinChange = 0;

void changeSkin(int id) {
    if (!enabled) return;

    uint32_t currentTime = GetTickCount();
    if (currentTime - lastSkinChange < 100) {
        sampapi::v037r3::RefChat()->AddMessage(-1, "[SkinChanger] {FF0000}Please wait before changing skin again{FF0000}");
        return;
    }

    if (playerId == 0) {
        sampapi::v037r3::RefChat()->AddMessage(-1, "[SkinChanger] {FF0000}Cannot restore skin: Player ID unknown{FF0000}");
        return;
    }

    lastSkinChange = currentTime;
    skinId = static_cast<uint32_t>(id);

    RakNet::BitStream rpcData;
    rpcData.Write<uint32_t>(playerId);
    rpcData.Write<uint32_t>(skinId);

    if (rakhook::emul_rpc(153, rpcData)) {
        char msg[64];
        sprintf(msg, "[SkinChanger] Skin changed to %d", skinId);
        sampapi::v037r3::RefChat()->AddMessage(-1, msg);
    }
}

void changeSkinComm(const char* c) {
    int id = atoi(c);
    if (id < 0) {
        return;
    }
    changeSkin(id);
}

void toggleChanger(const char* c) {
    enabled = !enabled;

    if (enabled) {
        sampapi::v037r3::RefChat()->AddMessage(-1, "[SkinChanger] SkinChanger {008000}enabled{008000}");
        changeSkin(skinId);
    }
    else {
        if (playerId == 0) {
            sampapi::v037r3::RefChat()->AddMessage(-1, "[SkinChanger] {FF0000}Cannot restore skin: Player ID unknown{FF0000}");
            return;
        }

        if (realSkinId == 0) {
            sampapi::v037r3::RefChat()->AddMessage(-1, "[SkinChanger] {FF0000}Cannot restore skin: Original skin unknown{FF0000}");
            return;
        }

        RakNet::BitStream rpcData;
        rpcData.Write<uint32_t>(playerId);
        rpcData.Write<uint32_t>(realSkinId);

        if (rakhook::emul_rpc(153, rpcData)) {
            sampapi::v037r3::RefChat()->AddMessage(-1, "[SkinChanger] {FF0000}disabled{FF0000}");
        }
        else {
            sampapi::v037r3::RefChat()->AddMessage(-1, "[SkinChanger] {FF0000}Failed to restore original skin{FF0000}");
            enabled = true;
        }
    }
}

void game_loop(game_loop_t orig) {
    orig();

    static bool initialized = false;
    if (initialized || !rakhook::initialize())
        return;

    StringCompressor::AddReference();

    rakhook::on_receive_rpc += [](unsigned char& id, RakNet::BitStream* bs) -> bool {
        if (id != 139) return true;

        int oldOffset = bs->GetReadOffset();
        bs->ResetReadPointer();

        bool bZoneNames;
        bool bUseCJWalk;
        bool bAllowWeapons;
        bool bLimitGlobalChatRadius;
        float fGlobalChatRadius;
        bool bStuntBonus;
        float fNameTagDistance;
        bool bDisableEnterExits;
        bool bNameTagLOS;
        bool bManualVehEngineAndLights;
        uint32_t dSpawnsAvailable;

        bs->Read<bool>(bZoneNames);
        bs->Read<bool>(bUseCJWalk);
        bs->Read<bool>(bAllowWeapons);
        bs->Read<bool>(bLimitGlobalChatRadius);
        bs->Read<float>(fGlobalChatRadius);
        bs->Read<bool>(bStuntBonus);
        bs->Read<float>(fNameTagDistance);
        bs->Read<bool>(bDisableEnterExits);
        bs->Read<bool>(bNameTagLOS);
        bs->Read<bool>(bManualVehEngineAndLights);
        bs->Read<uint32_t>(dSpawnsAvailable);

        bs->Read<uint16_t>(playerId);

        bs->SetReadOffset(oldOffset);
        return true;
    };

    rakhook::on_receive_rpc += [](unsigned char& id, RakNet::BitStream* bs) -> bool {
        if (id != 153) return true;
        if (playerId == 0) return true;

        int oldOffset = bs->GetReadOffset();
        bs->ResetReadPointer();

        uint32_t receivedPlayerId = 0;
        uint32_t receivedSkinId = 0;

        if (bs->Read<uint32_t>(receivedPlayerId) && bs->Read<uint32_t>(receivedSkinId)) {
            if (receivedPlayerId == playerId) {
                realSkinId = receivedSkinId;
                if (enabled) {
                    RakNet::BitStream modifiedBs;
                    modifiedBs.Write<uint32_t>(receivedPlayerId);
                    modifiedBs.Write<uint32_t>(skinId);

                    *bs = modifiedBs;
                }
            }
        }

        bs->SetReadOffset(oldOffset);

        return true;
    };

    rakhook::on_receive_packet += [](Packet* packet) -> bool {
        RakNet::BitStream bs(packet->data, packet->length, false);

        uint8_t id;
        bs.Read(id);

        if (id == 34) {
            sampapi::v037r3::RefChat()->AddMessage(-1, "SkinChanger by Talitet loaded");
            sampapi::v037r3::RefChat()->AddMessage(-1, "[SkinChanger] /chtog to toggle SkinChanger");
            sampapi::v037r3::RefChat()->AddMessage(-1, "[SkinChanger] /chskin [id] to change skin");
            sampapi::v037r3::RefChat()->AddMessage(-1, "[SkinChanger] <- / -> to set previous/next skin");
        }
            

        return true;
    };

    sampapi::v037r3::RefInputBox()->AddCommand("chskin", changeSkinComm);
    sampapi::v037r3::RefInputBox()->AddCommand("chtog", toggleChanger);

    initialized = true;
}

LRESULT wndproc_hooked(wndproc_t orig, HWND hwnd, UINT Message, WPARAM wparam, LPARAM lparam) {
    if (Message == WM_KEYDOWN) {
        if (wparam == VK_LEFT) {
            changeSkin(skinId - 1);
        }
        if (wparam == VK_RIGHT) {
            changeSkin(skinId + 1);
        }
    }
    return orig(hwnd, Message, wparam, lparam);
}

std::unique_ptr<cyanide::polyhook_x86<game_loop_t, decltype(&game_loop)>>    game_loop_hook;
std::unique_ptr<cyanide::polyhook_x86<wndproc_t, decltype(&wndproc_hooked)>> wndproc_hook;

using namespace plugin;

class Main {
public:
    Main() {
        if (!rakhook::samp_addr() || rakhook::samp_version() == rakhook::samp_ver::unknown)
            return;

        game_loop_hook = std::make_unique<typename decltype(game_loop_hook)::element_type>(std::bit_cast<game_loop_t>(0x53BEE0), std::move(&game_loop));
        wndproc_hook = std::make_unique<typename decltype(wndproc_hook)::element_type>(std::bit_cast<wndproc_t>(0x747EB0), std::move(&wndproc_hooked));

        game_loop_hook->install();
        wndproc_hook->install();
    }

    ~Main() {

    };
} g_Main;