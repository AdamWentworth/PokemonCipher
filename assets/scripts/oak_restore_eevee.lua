return {
    { "lock_input" },
    { "say", "OAK", "Ah, Wes. Let's get your partner ready again." },
    { "say", "OAK", "I'm restoring Eevee to full field condition." },
    { "clear_party" },
    { "add_party", 133, 5, true },
    { "set_flag", "starter_eevee_obtained", true },
    { "set_var", "partner_species_id", 133 },
    { "set_var", "partner_level", 5 },
    { "set_var", "story_checkpoint", 20 },
    { "say", "OAK", "Take Eevee and head back out when you're ready." },
    { "unlock_input" },
}
