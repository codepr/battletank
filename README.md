Battletank
==========
Dumb teminal based implementation of the battle tank game, the only dependency
is `ncurses`

## Why
To have some fun, small old school programs are fun to mess with.

## Build
```bash
make
```

## Ideas
In no particular order, and not necessarily mandatory:
- Implement a very simple and stripped down game logic ✅
    - 1 player, 1 tank, 1 bullet ✅
    - Handle keyboard input ✅
    - Design a better structured game state, multiple tanks, each tank loaded with (one or more) bullets ✅
- Multiplayer through TCP sockets
    - Implement a TCP server ✅
    - Implement a TCP client ✅
    - Implement a network protocol (text based or binary) ✅
    - The clients will send their input, server handle the game state and broadcasts it to the connected clients ✅
    - Heartbeat server side, drop inactive clients
    - Small chat? maybe integrate [chatlite](https://github.com/codepr/chatlite.git)
    - Ensure screen size scaling is maintained in sync with players
### Future improvements
- Walls
- Life points
- Bullets count
- Recharge bullets on a time basis
- Power ups (faster bullets? larger hit box? Mines? they could appear 1-2px before stepping on them)
- Explore SDL2 or Raylib for some graphic or sprites

## Main challenges

The game is pretty simple, the logic is absolutely elementary, just x, y axis as direction for both the
tank and the bullets. The main challenges are represented by keeping players in sync and ensure that
the battlefield size is correctly scaled for each one of them (not touched this yet).

For the communication part I chose the simplest and obiquitous solution, a non-blocking server on `select`
(yikes, maybe at least `poll`) and a timeout on read client, this way the main loops are not blocked
indefinitely and the game state can flow. There are certainly infinitely better ways to do it, but
avoiding threading and excessive engineered solutions was part of the scope.
