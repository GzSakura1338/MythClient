# HypixelAutoPlay

ClickGUI → Player → HypixelAutoPlay. The module defaults to disabled.

- **Mode:** Hypixel.
- **Delay:** 0–30 seconds, in 0.1-second increments; default **5 seconds**.

The module follows Rise 6.9.5's HypixelAutoPlay trigger: a server chat prompt
containing the case-sensitive text `play again?`, with a `/play ...` command in
its sibling components. It uses the server-provided game selection; it does not
choose a different game, send GG, or add an IP restriction.

Nova reads RUN_COMMAND click events directly instead of splitting component
debug strings. This also supports the different click-event representation in
26.3. Non-command click actions, command lookalikes and control characters are
ignored; Rise's no-dot filter is retained. Action-bar messages are ignored.

The configured delay is captured when the prompt arrives. Repeated pending
commands are merged without restarting the countdown. Commands are sent once
on the game thread through normal outgoing packet handling. Disable, disconnect,
join, respawn and leaving the world cancel the queue. Successful submission uses
Nova's existing notification HUD; it is not confirmation of server admission.

Mappings are in the AutoPlay section of Lunar189.inc, forge1201.inc and
minecraft263.inc. Lunar sends C01; modern clients use the connection's command
method without the leading slash. No Java source or new bytecode hook is added.

Validation: `Client/test/player/hypixelautoplay/run.ps1` checks the native queue,
module lifecycle, chat adapter, and mappings against local class files. Server
testing is performed by the user after injection.
