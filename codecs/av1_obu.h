lsmash_av1_specific_parameters_t *obu_av1_parse_seq_header
(
    lsmash_bs_t *bs,
    uint32_t length,
    uint32_t offset
);

uint8_t *obu_av1_assemble_sample
(
    uint8_t *packetbuf,
    uint32_t length,
    uint32_t *samplelength,
    int *issync
);
