# Lunar 1.8.9 account switching

Implemented on 2026-09-30 using Recaf MCP inspection of the local Lunar bake:
`72c13d03/d3098b28/bake.zip`.

## Integration

- The existing account page, Microsoft browser authorization, saved credentials,
  token validation and offline accounts are shared with Lunar 1.21.11.
- `AccountCompatibility` selects the verified Lunar account naming build and
  appends the five account redirects to the 1.8.9 core rules. Unknown account
  builds retain core hooks and disable account integration as one group.
- `AccountSession` reads the correct session identity for both versions. Lunar
  navigation callbacks use it too, so Nova sessions can open the singleplayer
  and multiplayer menus without reopening account login.
- The existing legacy tick event also runs in menus; no world/player is required
  to process account requests. Login, selection and refresh use Nova's existing
  worker and tick-thread commit flow.

## Verified vanilla/Lunar behavior

`net.minecraft.util.Session` takes four strings: username, UUID, token and type.
The accepted types are `legacy` and `mojang`; modern Microsoft access tokens use
`mojang` here. This version has no modern `UserApiService`, profile keys, social
manager or telemetry session objects.

Lunar's `Minecraft.bridge$setSession` replaces `session`, clears
`profileProperties`, and notifies Lunar with the previous session. Nova performs
the same state transition, using a prepared empty PropertyMap to preserve the
old cache for rollback. It pre-resolves JNI members, publishes session/cache,
verifies identity, then sends the notification. Failure restores session, cache,
Lunar selection and its UI model. Nova-owned sessions clear Lunar's previous
selected account so its refresh cannot overwrite the new identity.

Commits reject a changed original session, a running Lunar login, a loaded world,
an integrated server, a live connection, or a connecting/loading screen.
In 1.8.9 the Minecraft constructor already allocates an IntegratedServer at the
title screen. Only `isAnvilFileSet()` identifies a real save-backed server; a
non-null reference alone incorrectly disables every account action at startup.
This is also the check vanilla `loadWorld(null)` uses before stopping a server.
`myNetworkManager` is checked for an open channel because it can retain a closed
connection after leaving an integrated world.

## Validation

`test/accounts/run.ps1 -LunarJar <1.8.9 bake.zip>` checks the account boundaries,
OAuth callback validation, encrypted credential storage, the five actual Lunar
redirects, 28 account/session methods and seven Minecraft fields. It also audits
the combined legacy and account hooks (21 classes, 94 checks).

`NovaUiPreview --accounts-smoke` additionally types a sample token and clicks
Continue, asserting that the entire token request is submitted and the popup
closes. It also retains the in-world account restriction checks.

The existing `test/lunar12111/run.ps1` covers the modern mapping/hook regression.
Build target: `NovaClientBase`, Release. Output:
`Client/build-vs2026/Release/NovaClientBase.dll`.

No live account authorization, session mutation or injection was performed by
these tests. After injecting, test switching at the main menu, joining and
leaving a singleplayer world, then switching again. Microsoft accounts should
also be tested against an authenticated multiplayer server; offline accounts
remain subject to the server's normal authentication requirements.
