#ifndef VITA_AOE_PLATFORM_IMPORTS_H
#define VITA_AOE_PLATFORM_IMPORTS_H

/* Patched Rise of Rome 1.0a import thunks (jmp [IAT]) that sit in the
   game's code range, and what stands in for them in the port. The DLL
   and name come from EMPIRESX.EXE's import directory; ordinal imports
   are named from their call shapes:
   - 0x005223a6 VERSION.VerQueryValueA, 0x005223ac GetFileVersionInfoA,
     0x005223b2 GetFileVersionInfoSizeA: only the game's file version
     check calls them; the port has no Windows version resources.
   - 0x005223b8 DPLAYX ordinal 1, called (guid, &play, NULL) by 0x00424500
     and from the provider callback 0x00428540: DirectPlayCreate.
     0x005223c4 ordinal 2, called (0x00428540, context) by 0x00428380:
     DirectPlayEnumerateA. 0x005223be ordinal 4, called (NULL, &lobby,
     NULL, NULL, 0) by the lobby set-up: DirectPlayLobbyCreateA. The port's
     sessions go through the platform callbacks of net_lists.h and
     comm_handler.h instead.
   - 0x005223ca DSOUND ordinal 1, called (NULL, &sound, NULL) by
     0x004a25d0: DirectSoundCreate; the port's sound output is a
     platform backend.
   - 0x005223d0 DDRAW.DirectDrawCreate; the port draws into its own
     surfaces (draw_system.h).

   Dead code: 0x00521e68, 0x00521e6f, 0x00521e7c, 0x00521e8f, 0x00521f2f,
   0x00521fcf, 0x00522072 and 0x00522112 are entry points of an unrolled
   backwards dword/byte copy (ESI/EDI register passing, no frame). Their
   only references are the jump tables at 0x00521ce0..0x00521e67, and no
   code or data in the executable refers to those tables, so nothing
   reaches them; the port has no counterpart. */

#endif
