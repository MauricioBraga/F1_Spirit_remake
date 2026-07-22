/*
 * This file used to hold carEngineSoundEffect(): a Mix_RegisterEffect()
 * callback that manually resampled the engine sample's raw PCM buffer
 * (Mix_Chunk::abuf) once per audio callback to fake an RPM-driven pitch
 * shift.
 *
 * SDL3_mixer's new track-based API (MIX_*) has no per-channel raw mixing
 * callback of that kind, and MIX_Audio no longer exposes a raw PCM
 * buffer to poke at directly - but it does provide the same effect
 * natively via MIX_SetTrackFrequencyRatio(), which is driven once per
 * game frame from CCar::cycle() now (see CCar.cpp and
 * Sound_set_channel_pitch() in sound.cpp).
 *
 * This file is kept as a placeholder/build target so the CMake source
 * list doesn't need touching, but it no longer contains anything.
 */
