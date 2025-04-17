#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <syslog.h>

#include "gamefs.h"
#include "modules.h"

#define GAMEFS_OPT_KEY(t, p, v) { t, offsetof(struct options, p), v }

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
	{"as688_mlb", "688 Attack Sub *.mlb files", init_game_as688_mlb},
	{"anox_dat", "Anachrnoox *.dat", init_game_anox_dat},
	{"aod_dat", "Anvil of Dawn data files", init_game_aod_dat},
	{"arc_dat", "Arcanum *.dat files", init_game_arc_dat},
	{"artifex_cub", "Artifex Mundi Games *.cub files", init_game_artifex_cub},
	{"bgate_bif", "Baldurs Gate *.bif files", init_game_bgate_bif},
	{"bs_rarc", "Broken Sword Director's Cut RARC *.dat files", init_game_bs_rarc},
	{"cf_dat", "Cannon Fodder *.dat files", init_game_cf_dat},
	{"comm_dir", "Commandos *.dir files", init_game_comm_dir},
	{"dune2_pak", "Dune 2 *.pak files", init_game_dune2_pak},
	{"dk_dat", "Dungeon Keeper *.dat files", init_game_dk_dat},
	{"fall_dat", "Fallout *.dat files", init_game_fall_dat},
	{"fall2_dat", "Fallout 2 *.dat files", init_game_fall2_dat},
	{"fez_pak", "Fez *.pak files", init_game_fez_pak},
	{"fragall__", "Fragile Allegiance _* files", init_game_fragall__},
	{"ftl_dat", "Faster Than Light *.dat files", init_game_ftl_dat},
	{"gob_stk", "Goblins 1,2,3 & Geisha *.stk files", init_game_gob_stk},
	{"g17_dat", "Gorky 17 *.dat files", init_game_g17_dat},
	{"gta3_img", "Grand Theft Auto 3 *.[img|dir] files pair", init_game_gta3_img},
	{"hotmi_wad", "Hotline Miami *wad file", init_game_hotmi_wad},
	{"jagg_dat", "Jagged Alliance *.dat files", init_game_jagg_dat},
	{"ja2_slf", "Jagged Alliance *.slf files", init_game_ja2_slf},
	{"lfosh_lib", "Lost Files of Sherlock Holmes *.lib files", init_game_lfosh_lib},
	{"mm_lod", "Might&Magic VI-VIII *.lod files", init_game_mm_lod},
	{"mm_snd", "Might&Magic VI-VIII *.snd files", init_game_mm_snd},
	{"mm_vid", "Might&Magic VI-VIII *.vid files", init_game_mm_vid},
	{"nfs4_viv", "Need for Speed 4 *.viv files", init_game_nfs4_viv},
	{"risen_pak", "Risen *.pak files", init_game_risen_pak},
	{"sc2000_dat", "SimCity 2000 *.dat files", init_game_sc2000_dat},
	{"sforce_pak", "Spellforce *.pak files", init_game_sforce_pak},
	{"ss_res", "System Shock *.res files", init_game_ss_res},
	{"ta_hapi", "Total Anihilation HAPI files", init_game_ta_hapi},
	{"toee_dat", "Temple Of Elemental Evil *.dat files", init_game_toee_dat},
	{"tlj_xarc", "The Longest Journey *.xarc files", init_game_tlj_xarc},
	{"twow_wd", "Two Worlds *.wd files", init_game_twow_wd},
	{"ufoamh_vfs", "UFO Aftermath *.vgs files", init_game_ufoamh_vfs},
	{"ult7_dat", "Ultima 7 data files", init_game_ult7_dat},
	{"xeno_pfp", "Xenonauts *pfp files", init_game_xeno_pfp},
	{"", "", NULL}
};

static struct gametable othertable[] = {
	{"canon_fw", "Cannon printer firmware files", init_arch_canon_fw},
	{"", "", NULL}
};


void help(void) {
	int i;
	printf("\nAvailable games:\n");
	for (i = 0; gametable[i].game[0]; i++) {
		printf("\t%-16s - %s\n", gametable[i].game, gametable[i].description);
	}
	printf("\nOther supportrd archives:\n");
	for (i = 0; othertable[i].game[0]; i++) {
		printf("\t%-16s - %s\n", othertable[i].game, othertable[i].description);
	}
	printf("\n");
}

int main(int argc, char *argv[]) {
	int ret;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	openlog("GAMETOOL FUSE", 0, LOG_USER);

	memset(&fs->options, 0, sizeof(struct options));
	if (fuse_opt_parse(&args, &fs->options, gamefs_opts, NULL) == -1)
		return -1;

	if (fs->options.game) {
		for (int i = 0; gametable[i].game[0]; i++) {
			if (strcmp(fs->options.game, gametable[i].game) == 0) {
				ret = generic_initfs();
				if (ret) {
					return ret;
				}
				syslog(LOG_DEBUG, "Init \"%s\" VFS.\n", gametable[i].description);
				ret = gametable[i].initgame();
				if (ret) {
					return ret;
				}
				goto found;
			}
		}

		for (int i = 0; othertable[i].game[0]; i++) {
			if (strcmp(fs->options.game, othertable[i].game) == 0) {
				ret = generic_initfs();
				if (ret) {
					return ret;
				}
				syslog(LOG_DEBUG, "Init \"%s\" VFS.\n", othertable[i].description);
				ret = othertable[i].initgame();
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
