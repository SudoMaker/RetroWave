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

void RetroWavePlayer::regmap_insert(int idx, uint8_t reg, uint8_t val) {
	auto &v = reg_map[idx];
	if (v.size() < (reg+1)) {
		v.resize(reg+1);
	}

	v[reg] = val;

	reg_map_refreshed_list[idx].insert(reg);
}

void RetroWavePlayer::regmap_sn76489_insert(uint8_t data) {
	regmap_sn76489.used = true;

	uint8_t dat = retrowave_invert_byte(data);

	if (dat & 0x1) {
		regmap_sn76489.last_reg = (data >> 1) & 0x07;
	}

	uint8_t idx = 0, mode = 0; // 1: Freq, 2: Att

	switch (regmap_sn76489.last_reg) {
		case 0: // Tone 1 Freq
			mode = 1;
			idx = 0;
			break;
		case 2: // Tone 2 Freq
			mode = 1;
			idx = 1;
			break;
		case 1: // Tone 3 Freq
			mode = 1;
			idx = 2;
			break;

		case 4: // Tone 1 Att
			mode = 2;
			idx = 0;
			break;
		case 6: // Tone 2 Att
			mode = 2;
			idx = 1;
			break;
		case 5: // Tone 3 Att
			mode = 2;
			idx = 2;
			break;

		case 3: // Noise ctrl
			regmap_sn76489.noise_ctrl = (data >> 4) & 0x0f;
			break;
		case 7: // Noise att
			regmap_sn76489.noise_att = (data >> 4) & 0x0f;
			break;
	}

	if (mode == 1) { // Freq
		auto &val = regmap_sn76489.freq[idx];

		if (dat & 0x1) { // reg + upper 4 bits
			uint8_t val_upper = (data >> 4) & 0x0f; // Extract upper 4 bits
			val &= 0x3f; // Preserve lower 6 bits
			val |= val_upper << 6; // Merge
		} else { // lower 6 bits
			uint8_t val_lower = (data >> 2) & 0x3f; // Extract lower 6 bits
			val &= 0x3c0; // Preserve upper 4 bits
			val |= val_lower; // Merge
		}
	} else if (mode == 2) { // Att
		if (dat & 0x1) {
			regmap_sn76489.att[idx] = (data >> 4) & 0x0f;
		}
	}


}
