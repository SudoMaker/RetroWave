/*
    This file is part of RetroWave.

    Copyright (C) 2021 ReimuNotMoe <reimu@sudomaker.com>
    Copyright (C) 2021 Yukino Song <yukino@sudomaker.com>


    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "Player.hpp"

static std::unordered_map<int, std::string_view> regmap_name = {
	{0x5a, "OPL2"},
	{0x5e, "OPL3 Port0"},
	{0x5f, "OPL3 Port1"},
	{0x50, "SN76489"},
	{0x51, "YM2413"},
	{0xbd, "SAA1099"},
};

void RetroWavePlayer::osd_show_regmap_sn76489() {

	for (size_t i=0; i<sizeof(regmap_sn76489)/sizeof(SN76489Registers); i++) {
		auto &cur_regmap = regmap_sn76489[i];
		if (!cur_regmap.used) {
			continue;
		}

		const char *chip_id, *chip_chan;


		if (sn76489_dual) {
			if (i) {
				chip_chan = "Right";
				chip_id = "#1 ";
			} else {
				chip_chan = "Left";
				chip_id = "#0 ";
			}
		} else {
			if (i) {
				break;
			}

			chip_chan = "Mono";
			chip_id = "";
		}

		printf("Register map of SN76489 %s(%s):\033[K\n", chip_id, chip_chan);

		const char *col_freq[3] = {"", "", ""};
		const char *col_att[3] = {"", "", ""};
		const char *col_noise[2] = {"", ""};
		const char *col = "\033[01;32m";

		if (cur_regmap.is_volume) {
			switch (cur_regmap.channel) {
				case 0:
					// Tone 1 Att
					col_att[0] = col;
					break;
				case 1:
					// Tone 2 Att
					col_att[1] = col;
					break;
				case 2:
					// Tone 3 Att
					col_att[2] = col;
					break;
				case 3:
					// Noise Att
					col_noise[1] = col;
					break;
			}
		} else {
			switch (cur_regmap.channel) {
				case 0:
					// Tone 1 Freq
					col_freq[0] = col;
					break;
				case 1:
					// Tone 2 Freq
					col_freq[1] = col;
					break;
				case 2:
					// Tone 3 Freq
					col_freq[2] = col;
					break;
				case 3:
					// Noise Ctrl
					col_noise[0] = col;
					break;
			}
		}

		double cbuf[3];

		for (size_t j=0; j<3; j++) {
			cbuf[j] = 3579545.0 / (32 * cur_regmap.freq[j]);
		}

		printf(
			"Freq: %s0x%03x\033[0m %s0x%03x\033[0m %s0x%03x\033[0m | "
			"%s%10.3lf\033[0m %s%10.3lf\033[0m %s%10.3lf\033[0m"
			"\033[K\n",
			col_freq[0], cur_regmap.freq[0],
			col_freq[1], cur_regmap.freq[1],
			col_freq[2], cur_regmap.freq[2],

			col_freq[0], cbuf[0],
			col_freq[1], cbuf[1],
			col_freq[2], cbuf[2]
		);

		char sbuf[3][16];

		for (size_t j=0; j<3; j++) {
			if (cur_regmap.att[j] == 0xf) {
				sprintf(sbuf[j], "     Mute");
			} else {
				sprintf(sbuf[j], "  %4d dB", -(cur_regmap.att[j] * 2));
			}
		}

		printf(
			"Att : %s0x%x\033[0m   %s0x%x\033[0m   %s0x%x\033[0m   |  "
			"%s%s\033[0m  %s%s\033[0m  %s%s\033[0m"
			"\033[K\n",
			col_att[0], cur_regmap.att[0],
			col_att[1], cur_regmap.att[1],
			col_att[2], cur_regmap.att[2],

			col_att[0], sbuf[0],
			col_att[1], sbuf[1],
			col_att[2], sbuf[2]
		);

		const char *fb_str;

		if ((cur_regmap.noise_ctrl >> 2) & 0x1) {
			fb_str = "     White";
		} else {
			fb_str = "  Periodic";
		}

		uint8_t sr = cur_regmap.noise_ctrl & 0x3;
		const char *sr_str;

		switch (sr) {
			case 0:
				sr_str = "   N/512";
				break;
			case 1:
				sr_str = "  N/1024";
				break;
			case 2:
				sr_str = "  N/2048";
				break;
			case 3:
				sr_str = "    Tone";
				break;
			default:
				sr_str = "";
				break;
		}

		if (cur_regmap.att[3] == 0xf) {
			sprintf(sbuf[2], "     Mute");
		} else {
			sprintf(sbuf[2], "  %4d dB", -(cur_regmap.att[3] * 2));
		}

		printf(
			"Noise ctrl: %s0x%x         | %s   %s\033[0m\033[K\n",
			col_noise[0], cur_regmap.noise_ctrl,
			fb_str,
			sr_str
		);

		printf("Noise att : %s0x%x\033[0m         |  "
		       "%s%s\033[0m"
		       "\033[K\n", col_noise[1], cur_regmap.att[3], col_noise[1], sbuf[2]);

		printf("\033[K\n\033[K\n");
	}

}

void RetroWavePlayer::osd_show_regmaps() {
	for (auto &it : reg_map) {
		const char *rmn = "?";
		auto rmnit = regmap_name.find(it.first);
		if (rmnit != regmap_name.end())
			rmn = rmnit->second.data();

		printf("Register map %02x [%s]:\033[K\n", it.first, rmn);

		auto &refreshed_list = reg_map_refreshed_list[it.first];

		for (int i=0; i<it.second.size(); i+=16) {
			printf("%08x  ", i);


			for (int j=0; j<16; j++) {
				if (refreshed_list.find(i+j) != refreshed_list.end())
					printf("\033[01;32m%02x\033[0m ", it.second[i+j]);
				else
					printf("%02x ", it.second[i+j]);
			}
			printf("\033[K\n");
		}

		reg_map_refreshed_list[it.first].clear();
		printf("\033[K\n\033[K\n");
	}
}

void RetroWavePlayer::osd_show_metadata() {
	if (!metadata.title.empty()) {
		printf("Title: %s", metadata.title.c_str());
		if (!metadata.title_jp.empty())
			printf(" / %s", metadata.title_jp.c_str());
		printf("\033[K\n");
	}

	if (!metadata.album.empty()) {
		printf("Album: %s", metadata.album.c_str());
		if (!metadata.album_jp.empty())
			printf(" / %s", metadata.album_jp.c_str());
		printf("\033[K\n");
	}

	if (!metadata.composer.empty()) {
		printf("Composer: %s", metadata.composer.c_str());
		if (!metadata.composer_jp.empty())
			printf(" / %s", metadata.composer_jp.c_str());
		printf("\033[K\n");
	}

	if (!metadata.system_name.empty()) {
		printf("System: %s", metadata.system_name.c_str());
		if (!metadata.system_name_jp.empty())
			printf(" / %s", metadata.system_name_jp.c_str());
		printf("\033[K\n");
	}

	if (!metadata.release_date.empty())
		printf("Release date: %s\033[K\n", metadata.release_date.c_str());

	if (!metadata.converter.empty())
		printf("Converter: %s\033[K\n", metadata.converter.c_str());

	if (!metadata.note.empty())
		printf("Note: %s\033[K\n", metadata.note.c_str());

	printf("\033[K\n");
}

void RetroWavePlayer::osd_show() {
	term_move_0_0();
	printf("===== RetroWave Player =====\033[K\n\033[K\n");

	printf("Now playing (%zu/%zu): %s\033[K\n\033[K\n", current_track, total_tracks, current_file);

	if (osd_show_meta)
		osd_show_metadata();

	if (osd_show_regs) {
		osd_show_regmaps();
		osd_show_regmap_sn76489();
	}

	auto [h, m, s] = sec2hms(played_samples / sample_rate);

	const char *samples_color = "\033[01;32m"; // Green

	if (last_slept_samples % 2) {
		samples_color = "\033[01;33m";
	}

	if (labs((long)last_last_slept_samples - (long)last_slept_samples) == 1) {
		samples_color = "\033[01;31m";
	}

	if (s - last_secs >= 1) {
		bytes_per_sec = played_bytes;
		played_bytes = 0;
		last_secs = s;
	}

	printf("Bandwidth: %06.4lf KiB/s\033[K\n\033[2K", (double)bytes_per_sec / 1000);

	printf("\n");

	double last_slept_msecs = (double)last_slept_usecs / 1000000.0;
	double fps = 1000.0 / last_slept_msecs;

	if (total_samples) {
		auto [th, tm, ts] = sec2hms(total_samples / sample_rate);
		printf("Playing: %02d:%02d:%02d / %02d:%02d:%02d +%011.6lfms (%zu/%zu %s+%05zu\033[0m %06.3lf)\033[K\n", h, m, s, th, tm, ts, last_slept_msecs, played_samples, total_samples, samples_color, last_slept_samples, fps);
	} else {
		printf("Playing: %02d:%02d:%02d +%011.6lfms (%zu %s+%05zu\033[0m %06.3lf)\033[K\n", h, m, s, last_slept_msecs, played_samples, samples_color, last_slept_samples, fps);
	}

	last_last_slept_samples = last_slept_samples;
}




