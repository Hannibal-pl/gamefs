#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <syslog.h>

#include "gamefs.h"
#include "modules.h"

#define GAMEFS_OPT_KEY(t, p, v)	{ t, offsetof(struct options, p), v }
#define ARRAY_SIZE(x)		(sizeof((x))/sizeof((x)[0]))
#define GAMETABLE_SPACER	"SpAcEr"

enum {
	KEY_VERSION,
	KEY_HELP,
};

static struct fuse_opt gamefs_opts[] = {
	GAMEFS_OPT_KEY("game=%s", game, 0),
	GAMEFS_OPT_KEY("--game %s", game, 0),
	GAMEFS_OPT_KEY("param=%s", param, 0),
	GAMEFS_OPT_KEY("--param %s", param, 0),
	GAMEFS_OPT_KEY("file=%s", file, 0),
	GAMEFS_OPT_KEY("--file %s", file, 0),
	FUSE_OPT_END
};

static struct gametable gametable[] = {
	{"as688_mlb", "688 Attack Sub *.mlb files", init_game_as688_mlb, detect_game_as688_mlb},
	{"anox_dat", "Anachrnoox *.dat", init_game_anox_dat, detect_game_anox_dat},
	{"aod_dat", "Anvil of Dawn data files", init_game_aod_dat, detect_game_aod_dat},
	{"arc_dat", "Arcanum *.dat files", init_game_arc_dat, detect_game_arc_dat},
	{"artifex_cub", "Artifex Mundi Games *.cub files", init_game_artifex_cub, detect_game_artifex_cub},
	{"bards_gob", "The Bard's Tale *.gob files", init_game_bards_gob, detect_game_bards_gob},
	{"bards_lmp", "The Bard's Tale *.lmp files", init_game_bards_lmp, detect_game_bards_lmp},
	{"bgate_bif", "Baldurs Gate *.bif files", init_game_bgate_bif, detect_game_bgate_bif},
	{"bs_rarc", "Broken Sword Director's Cut RARC *.dat files", init_game_bs_rarc, detect_game_bs_rarc},
	{"cf_dat", "Cannon Fodder *.dat files", init_game_cf_dat, detect_game_cf_dat},
	{"comm_dir", "Commandos *.dir files", init_game_comm_dir, detect_game_comm_dir},
	{"doom_wad", "Doom engine *.wad files", init_game_doom_wad, detect_game_doom_wad},
	{"duke3d_grp", "Duke3D *.grp file", init_game_duke3d_grp, detect_game_duke3d_grp},
	{"dune2_pak", "Dune 2 *.pak files", init_game_dune2_pak, detect_game_dune2_pak},
	{"dk_dat", "Dungeon Keeper *.dat files", init_game_dk_dat, detect_game_dk_dat},
	{"dk2_dat", "Dungeon Keeper 2 *.wad files", init_game_dk2_wad, detect_game_dk2_wad},
	{"fall_dat", "Fallout *.dat files", init_game_fall_dat, detect_game_fall_dat},
	{"fall2_dat", "Fallout 2 *.dat files", init_game_fall2_dat, detect_game_fall2_dat},
	{"fez_pak", "Fez *.pak files", init_game_fez_pak, detect_game_fez_pak},
	{"fragall__", "Fragile Allegiance _* files", init_game_fragall__, detect_game_fragall__},
	{"ftl_dat", "Faster Than Light *.dat files", init_game_ftl_dat, detect_game_ftl_dat},
	{"gob_stk", "Goblins 1,2,3 & Geisha *.stk files", init_game_gob_stk, detect_game_gob_stk},
	{"g17_dat", "Gorky 17 *.dat files", init_game_g17_dat, detect_game_g17_dat},
	{"gta3_img", "Grand Theft Auto 3 *.[img|dir] files pair", init_game_gta3_img, detect_game_gta3_img},
	{"h3_lod", "Heroes of M&M 3 *.lod file", init_game_h3_lod, detect_game_h3_lod},
	{"hotmi_wad", "Hotline Miami *.wad file", init_game_hotmi_wad, detect_game_hotmi_wad},
	{"jagg_dat", "Jagged Alliance *.dat files", init_game_jagg_dat, detect_game_jagg_dat},
	{"ja2_slf", "Jagged Alliance *.slf files", init_game_ja2_slf, detect_game_ja2_slf},
	{"lfosh_lib", "Lost Files of Sherlock Holmes *.lib files", init_game_lfosh_lib, detect_game_lfosh_lib},
	{"mm_hwl", "Might&Magic VII-VIII *.hwl files", init_game_mm_hwl, detect_game_mm_hwl},
	{"mm_lod", "Might&Magic VI-VIII *.lod files", init_game_mm_lod, detect_game_mm_lod},
	{"mm_snd", "Might&Magic VI-VIII *.snd files", init_game_mm_snd, detect_game_mm_snd},
	{"mm_vid", "Might&Magic VI-VIII *.vid files", init_game_mm_vid, detect_game_mm_vid},
	{"nfs4_viv", "Need for Speed 4 *.viv files", init_game_nfs4_viv, detect_game_nfs4_viv},
	{"nolf_rez", "No One Lives Forever 1&2 *.rez files", init_game_nolf_rez, detect_game_nolf_rez},
	{"risen_pak", "Risen *.pak files", init_game_risen_pak, detect_game_risen_pak},
	{"sc2000_dat", "SimCity 2000 *.dat files", init_game_sc2000_dat, detect_game_sc2000_dat},
	{"sforce_pak", "Spellforce *.pak files", init_game_sforce_pak, detect_game_sforce_pak},
	{"ss_res", "System Shock *.res files", init_game_ss_res, detect_game_ss_res},
	{"ta_hapi", "Total Anihilation HAPI files", init_game_ta_hapi, detect_game_ta_hapi},
	{"toee_dat", "Temple Of Elemental Evil *.dat files", init_game_toee_dat, detect_game_toee_dat},
	{"tlj_xarc", "The Longest Journey *.xarc files", init_game_tlj_xarc, detect_game_tlj_xarc},
	{"twow_wd", "Two Worlds *.wd files", init_game_twow_wd, detect_game_twow_wd},
	{"ufoamh_vfs", "UFO Aftermath *.vgs files", init_game_ufoamh_vfs, detect_game_ufoamh_vfs},
	{"ult7_dat", "Ultima 7 data files", init_game_ult7_dat, detect_game_ult7_dat},
	{"xeno_pfp", "Xenonauts *pfp files", init_game_xeno_pfp, detect_game_xeno_pfp},
	{GAMETABLE_SPACER, GAMETABLE_SPACER, NULL, NULL},
	{"canon_fw", "Cannon printer firmware files", init_arch_canon_fw, detect_arch_canon_fw},
	{"elwo_res", "Electronic Workbench 4 *.res, *.ewx files", init_arch_elwo_res, detect_arch_elwo_res},
	{"", "", NULL, NULL}
};


void help(void) {
	int i;
	printf("\nAvailable games:\n");
	for (i = 0; gametable[i].game[0]; i++) {
		if (strcmp(GAMETABLE_SPACER, gametable[i].game) == 0) {
			printf("\nOther supported archives:\n");
			continue;
		}
		printf("\t%-16s - %s\n", gametable[i].game, gametable[i].description);
	}
	printf("\nOr use `auto` for attempt to autodetect archive type.\n");
	printf("\n");
}

uint32_t autodetect(uint32_t starti) {
	for (uint32_t i = starti; i < ARRAY_SIZE(gametable) - 1; i++) {
		if ((gametable[i].autodetect != NULL) && gametable[i].autodetect()) {
			return i;
		}
	}

	return 0xFFFFFFFF;
}

int main(int argc, char *argv[]) {
	int ret;
	uint32_t match = 0xFFFFFFFF;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	openlog("GAMETOOL FUSE", 0, LOG_USER);

	memset(&fs->options, 0, sizeof(struct options));
	if (fuse_opt_parse(&args, &fs->options, gamefs_opts, NULL) == -1)
		return -1;

	if (fs->options.game) {
		if (strcmp(fs->options.game, "auto") == 0) {
			uint32_t matches = 0;

			printf("\nTrying to autodetect archive type...\n");

			ret = generic_initfs();
			if (ret) {
				return ret;
			}

			for (uint32_t i = 0; i < ARRAY_SIZE(gametable) - 1;) {
				uint32_t lmatch = autodetect(i);
				if (lmatch != 0xFFFFFFFF) {
					printf("Match found: %s (%s)\n", gametable[lmatch].game, gametable[lmatch].description);
					matches++;
					match = lmatch;
					i = lmatch + 1;
				} else {
					break;
				}
			}

			if (matches == 1) {
				goto found;
			}

			printf("\nNone or more than one match found. Exiting\n");
			generic_closefs();
			return -1;
		}

		for (int i = 0; gametable[i].game[0]; i++) {
			if (strcmp(GAMETABLE_SPACER, gametable[i].game) == 0) {
				continue;
			}

			if (strcmp(fs->options.game, gametable[i].game) == 0) {
				match = i;

				ret = generic_initfs();
				if (ret) {
					return ret;
				}
				goto found;
			}
		}

		fprintf(stderr, "Unknown game type.\n");
		return -EINVAL;
	} else {
		printf("You must choose game file type.\n");
		printf("Usage:\n\t%s --game [GAME_TYPE] --file [ARCHIVE_FILE] [--param OPTIONAL_PARAMETER] [MOUNT_POINT]\n", argv[0]);
		help();
		return -EINVAL;
	}

found:
//	fprintf(stderr, "MATCH %i\n", match);

	syslog(LOG_DEBUG, "Init \"%s\" VFS.\n", gametable[match].description);
	ret = gametable[match].initgame();
	if (ret) {
		return ret;
	}

	generic_print_tree(fs->root, 0);

	ret = fuse_main(args.argc, args.argv, &fs->operations, NULL);
	if (ret) {
		fprintf(stderr, "FUSE init error.\n");
	}

	generic_closefs();
	syslog(LOG_DEBUG, "Close VFS.\n");
	closelog();

	fuse_opt_free_args(&args);
	return ret;
}
