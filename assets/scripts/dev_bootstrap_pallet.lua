return {
    { "lock_input" },
    { "log", "Bootstrapping Pallet Town test state from Lua." },
    { "warp_spawn", "pallet_town", "default" },
    { "set_flag", "intro_complete", true },
    { "set_var", "story_checkpoint", 1 },
    { "wait", 0.2 },
    { "unlock_input" },
}
