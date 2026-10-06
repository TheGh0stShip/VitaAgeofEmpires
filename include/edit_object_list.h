#ifndef VITA_AOE_EDIT_OBJECT_LIST_H
#define VITA_AOE_EDIT_OBJECT_LIST_H

#include "player_templates.h"
#include "ui_dialog.h"

#include <stdint.h>

/* The scenario editor's object list dialog (vtable 0x00545b84,
   0x0046a890..0x0046ad40), a list dialog of the viewed player's
   templates. */

/* One 0x66-byte entry: the shown name and the template slot index (-1 for
   the leading " <None> " entry). */
typedef struct {
    char name[100];
    int16_t id; /* +0x64 */
} AoeEditObjectEntryRor;

typedef struct {
    void *context;
    /* The viewed player's templates (0x00419b70 +0x22 / +0x24); NULL
       without a viewed player. */
    const AoePlayerTemplates *(*templates)(void *context);
    /* Game vtable +0x1c(2, language ID, out, 100): an object name from the
       string table. */
    void (*object_name)(void *context, int16_t language_id, char *out,
                        int size);
    /* Game +0x3fc: the object being placed. */
    int16_t (*place_object)(void *context);
} AoeEditObjectListOpsRor;

typedef struct {
    AoeUiListDialogRor list;
    int32_t mode;                   /* +0x4ac: 1 lists the templates */
    int16_t count;                  /* +0x4b0 */
    int16_t default_index;          /* +0x4b2 */
    AoeEditObjectEntryRor *entries; /* +0x4b4 */
    char **lines;                   /* +0x4b8 */
    const AoeEditObjectListOpsRor *list_ops;
} AoeEditObjectListRor;

extern const AoeUiEzVtblRor aoe_edit_object_list_vtable_ror;

/* 0x0046a890 constructor (the list dialog's, then the vtable). */
int aoe_edit_object_list_init_ror(AoeEditObjectListRor *d,
                                  AoeUiSystemRor *system,
                                  const AoeUiEzOpsRor *ops,
                                  const AoeEditObjectListOpsRor *list_ops,
                                  const char *name);

/* 0x0046a8d0 setup: the mode stored and both arrays cleared; fails when
   panel +0xd8 is set or the entries cannot be built, then sorts them,
   finds the default line, copies each name into its own line and lays out
   a 310 x 400 list dialog: title (10, 10, 290 x 25), list (10, 45,
   290 x 305), OK at x 10 and Cancel at x 160, y 360, 140 x 30. */
int aoe_edit_object_list_setup_ror(AoeEditObjectListRor *d,
                                   AoeUiPanelRor *parent,
                                   const char *info_name, int32_t info_id,
                                   int32_t mode, const char *title);

/* vtable +0x13c 0x0046ab00: in mode 1 with a viewed player, " <None> "
   (ID -1) then, in slot order, every present template whose +0x4e byte is
   zero, named by its language ID (+0x0c) when that is positive, else by
   its own name; the entry ID is the slot index. Returns 0 otherwise. */
int aoe_edit_object_list_build_ror(AoeEditObjectListRor *d);
/* 0x0046acb0: qsort of the entries by name with comparator 0x00492c30
   (CompareStringA, user locale, ignoring case). */
void aoe_edit_object_list_sort_ror(AoeEditObjectListRor *d);
/* 0x0046acd0: the first entry whose ID is the default ID, else 0. */
void aoe_edit_object_list_find_default_ror(AoeEditObjectListRor *d);
/* vtable +0x140 0x0046ad40: game +0x3fc in mode 1, else -1. */
int16_t aoe_edit_object_list_default_id_ror(const AoeEditObjectListRor *d);
/* vtable +0x134 0x0046aad0: the chosen line's ID, -1 with none. */
int16_t aoe_edit_object_list_choice_ror(const AoeEditObjectListRor *d);
/* 0x0046aa40 destructor: the lines and entries freed, then the list
   dialog's 0x00433900 (deleting 0x0046a8b0). */
void aoe_edit_object_list_release_ror(AoeEditObjectListRor *d);

#endif
