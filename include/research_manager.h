#ifndef VITA_AOE_RESEARCH_MANAGER_H
#define VITA_AOE_RESEARCH_MANAGER_H

#include <stdint.h>

#include "aoe_stream.h"
#include "dat.h"
#include "research_action.h"

/* Patched Rise of Rome 1.0a research manager accessors, load and save
   (0x004ea71a..0x004eba87). The manager (owner +0x224) keeps one
   eight-byte entry per tech (progress, state: -1 disabled, 0 locked,
   1 available, 2 researching, 3 done) over the tech table. */

/* Tech table: 0x004ea71a help string id, 0x004ea753 tech-tree string id,
   0x004ea78c hotkey; -1 outside the table (the original also reads the
   entry one past the end; the port returns -1 there). */
int32_t aoe_tech_help_ror(const DatTechs *techs, int16_t id);
int32_t aoe_tech_tree_text_ror(const DatTechs *techs, int16_t id);
int32_t aoe_tech_hotkey_ror(const DatTechs *techs, int16_t id);

/* 0x004ea7c5: read the entries (int16 count; per entry a 4-byte progress,
   or 2 bytes before save version 7.03 into its low half, and an int16
   state). With `reapply`, every done tech is made available and
   completed again, then the manager rebuilds. 0x004ea982 frees the
   entries; 0x004eb59f writes them (4-byte progress). */
int aoe_research_manager_load_ror(AoeResearchCompletionManagerRor *manager,
                                  AoeOutcomeStream *in, float save_version,
                                  int reapply);
void aoe_research_manager_release_ror(AoeResearchCompletionManagerRor *manager);
void aoe_research_manager_save_ror(const AoeResearchCompletionManagerRor *manager,
                                   AoeOutcomeStream *out);

/* 0x004ea9ac: a tech's state (-1 outside). 0x004eb61f: percent done
   (researching: progress * 100 / research time, truncated; done 100;
   else 0). */
int16_t aoe_research_state_ror(const AoeResearchCompletionManagerRor *manager,
                               int16_t id);
int16_t aoe_research_percent_ror(const AoeResearchCompletionManagerRor *manager,
                                 const DatTechs *techs, int16_t id);

/* Per tech, through the manager's range: 0x004eb6b9 name (NULL outside),
   0x004eb6f3 language name id, 0x004eb730 language description id,
   0x004eb76d button (0 outside); 0x004eba15 / 0x004eba4e / 0x004eba87
   help, tree text and hotkey (-1 outside). */
const char *aoe_research_name_ror(const AoeResearchCompletionManagerRor *manager,
                                  const DatTechs *techs, int16_t id);
uint16_t aoe_research_language_name_ror(
    const AoeResearchCompletionManagerRor *manager, const DatTechs *techs,
    int16_t id);
uint16_t aoe_research_language_description_ror(
    const AoeResearchCompletionManagerRor *manager, const DatTechs *techs,
    int16_t id);
uint8_t aoe_research_button_ror(const AoeResearchCompletionManagerRor *manager,
                                const DatTechs *techs, int16_t id);
int32_t aoe_research_help_ror(const AoeResearchCompletionManagerRor *manager,
                              const DatTechs *techs, int16_t id);
int32_t aoe_research_tree_text_ror(
    const AoeResearchCompletionManagerRor *manager, const DatTechs *techs,
    int16_t id);
int32_t aoe_research_hotkey_ror(const AoeResearchCompletionManagerRor *manager,
                                const DatTechs *techs, int16_t id);

/* 0x004eabc1: the costs to show for a tech: up to three (resource type,
   amount) pairs, type -1 for none. In the gold-reserve mode (owner
   resource 29 at least 1.0) a tech that can start shows each cost the
   owner cannot cover at what the owner has, the total shortfall moving
   into the first cost slot that is gold (3) or empty (the third slot
   takes it without adding). Amounts below 1 drop their type. */
void aoe_research_costs_ror(const AoeResearchOwnerRor *owner, int16_t id,
                            int16_t types[3], int16_t amounts[3]);

/* 0x004b6420 (the research action's readout): the tech (+0x40), its
   percent (100 once the action is in state 1, else 0x004eb61f) and, when
   asked, its name copied into `size` bytes (cut and terminated). */
void aoe_research_action_readout_ror(const AoeResearchCompletionManagerRor *manager,
                                     const DatTechs *techs, uint8_t action_state, int16_t tech,
                                     int16_t *id, int16_t *percent, char *name, int16_t size);

#endif
