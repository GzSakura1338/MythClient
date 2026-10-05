# Lunar account mappings: October 2, 2026

Source: local `offline/multiver/cache/d1937f9/cc81b79c/bake.zip`,
inspected with Recaf MCP alongside the updated `offline/multiver/lunar.jar`.

Account classes retain the September 30 names, but 16 method identifiers used
by Nova changed. `AccountMapping.h` adds an October 2 table keyed by the original
canonical identifiers. Runtime selection probes this build first and retains
both older tables. JNI calls and hook rules use the same selected table.

The multiplayer screen event also changed its parameter type independently of
the account-screen enum. `AccountEdits.h` uses the verified event descriptor for
this build only; replacing their shared old name globally would break navigation.

Recaf confirmed the `addAccount` and `selectAccount` JS callbacks, Xbox refresh
dispatch, authenticated-screen navigation, multiplayer event guard, account
manager, model map and session notification. The five native redirects keep
their existing behavior.

Validation against the new bake:

- 20 account JNI methods and all five account redirects pass native class-file
  lookup, rewrite, reparse, branch and stack-map checks.
- Combined modern hooks: 21 classes, 123 checks, zero failures.
- Modern JNI catalog: 761 bindings, zero failures.
- September 27 and September 30 account tables still pass. Lunar 1.8.9's
  September 30 bake also passes 28 JNI methods, session fields and five redirects.

These checks do not execute live account authorization or session switching.
