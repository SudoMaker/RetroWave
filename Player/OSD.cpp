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
	if (!regmap_sn76489.used)
		return;

	printf("Register map of SN76489:\033[K\n\033[K\n");

	const char *reg_desc = "";

	switch (regmap_sn76489.last_reg) {
		case 0:
			reg_desc = "Tone 1 Freq";
			break;
		case 2:
			reg_desc = "Tone 2 Freq";
			break;
		case 1:
			reg_desc = "Tone 3 Freq";
			break;
		case 4:
			reg_desc = "Tone 1 Att";
			break;
		case 6:
			reg_desc = "Tone 2 Att";
			break;
		case 5:
			reg_desc = "Tone 3 Att";
			break;
		case 3:
			reg_desc = "Noise Ctrl";
			break;
		case 7:
			reg_desc = "Noise Att";
			break;
	}

	printf("Last reg: 0x%x [%s]\033[K\n", regmap_sn76489.last_reg, reg_desc);
	printf("Freq: 0x%03x 0x%03x 0x%03x\033[K\n", regmap_sn76489.freq[0], regmap_sn76489.freq[1], regmap_sn76489.freq[2]);
	printf("Att : 0x%x   0x%x   0x%x\033[K\n", regmap_sn76489.att[0], regmap_sn76489.att[1], regmap_sn76489.att[2]);
	printf("Noise ctrl: 0x%x\033[K\n", regmap_sn76489.noise_ctrl);
	printf("Noise att : 0x%x\033[K\n", regmap_sn76489.noise_att);

	printf("\033[K\n\033[K\n");
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

	printf("Bandwidth: %06.4lf KiB/s\033[K\n", (double)bytes_per_sec / 1000);

	printf("\n");

	if (total_samples) {
		auto [th, tm, ts] = sec2hms(total_samples / sample_rate);
		printf("Playing: %02d:%02d:%02d / %02d:%02d:%02d +%011.6lfms (%zu/%zu %s+%05zu\033[0m)\033[K\n", h, m, s, th, tm, ts, ((double)last_slept_usecs / 1000000.0), played_samples, total_samples, samples_color, last_slept_samples);
	} else {
		printf("Playing: %02d:%02d:%02d +%011.6lfms (%zu %s+%05zu\033[0m)\033[K\n", h, m, s, ((double)last_slept_usecs / 1000000.0), played_samples, samples_color, last_slept_samples);
	}

	last_last_slept_samples = last_slept_samples;
}




