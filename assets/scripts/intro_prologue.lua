return {
    { "lock_input" },
    { "warp_spawn", "pallet_town", "default" },
    { "wait", 0.2 },

    { "say", "OAK", "Hello there! Glad to meet you." },
    { "say", "OAK", "My name is OAK. People call me the POKEMON PROFESSOR." },
    { "say", "OAK", "I heard you walked away from Team Snagem after what they became." },
    { "say", "WES", "Their corruption knew no bounds. They crossed a line." },
    { "say", "OAK", "Then walk the straight and narrow from this day on." },
    { "say", "OAK", "Take this Eevee. Trust your partner and start your journey." },

    { "clear_party" },
    { "add_party", 133, 5, true },
    { "set_flag", "former_snagem_recruit", true },
    { "set_flag", "starter_eevee_obtained", true },
    { "set_var", "partner_species_id", 133 },
    { "set_var", "partner_level", 5 },
    { "set_var", "story_checkpoint", 10 },
    { "set_flag", "intro_complete", true },

    { "say", "OAK", "Your very own POKEMON legend begins now!" },
    { "unlock_input" },
}
