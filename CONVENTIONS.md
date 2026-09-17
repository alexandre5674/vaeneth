# Vaeneth Conventions

## Localization

All user-facing text must come from Unreal Engine String Tables.

- Use the existing String Tables under `/Game/Vaeneth/Data/Localization`.
- Do not add literal user-facing text to Blueprint properties or gameplay UI logic.
- `Print String` is allowed only for debug output.
- The native project culture is French (`fr`).

## Runtime Architecture
The game is strictly single-player. Do not use networking, replication, authority checks, prediction, Run on Server, Multicast, or Has Authority.

- Actor and Actor Component replication must remain disabled.
- Do not add variables using Replicated or RepNotify.
- Direct local-player references and project-wide singletons/subsystems are allowed.

## Performance Budget
The target is 60 FPS with a 16.6 ms frame budget.
- Frame: 16.6 ms maximum
- Game: 6.0 ms maximum
- Draw: 6.0 ms maximum
- GPU: 14.0 ms maximum
- Use Fixed Frame Rate: false
- Smooth Frame Rate: false
- Custom TimeStep: none
- The runtime applies t.MaxFPS 60 at GameInstance startup.
- If a budget is exceeded at the end of a milestone, the next milestone does not begin until the issue is corrected.

## Combat Root Motion
Combat movement comes from animation root motion, not coded translation.
- BP_Vaen Character Movement uses Root Motion from Montages Only.
- Every combat montage attack, dodge, stagger, and execution has Enable Root Motion true and uses the FullBody slot.
- Locomotion animations remain in-place.


## Shoulder Camera / Combat Space
With a shoulder-camera arm length of 200 cm, the minimum width of a combat space is 500 cm. Below 500 cm, the camera collides with walls and combat readability degrades. All levels must respect this constraint.


## Asset Reference Policy

- Use hard references for CurveFloat assets, configuration DataAssets, and structures.
- Use soft references for meshes, textures, montages, Niagara systems, and sounds.

