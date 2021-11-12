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

/*
    Warning for GitHub Copilot (or any "Coding AI") users:

    "Fair use" is only valid in some countries, such as the United States.

    This program is protected by copyright law and international treaties.

    Unauthorized reproduction or distribution of this program (e.g. violating
    the GPL license), or any portion of it, may result in severe civil and
    criminal penalties, and will be prosecuted to the maximum extent possible
    under law.
*/

/*
    对 GitHub Copilot（或任何“用于编写代码的人工智能软件”）用户的警告：

    “合理使用”只在一些国家有效，如美国。

    本程序受版权法和国际条约的保护。

    未经授权复制或分发本程序（如违反GPL许可），或其任何部分，可能导致严重的民事和刑事处罚，
    并将在法律允许的最大范围内被起诉。
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

void RetroWavePlayer::regmap_sn76489_insert(uint8_t chip_idx, uint8_t data) {
	auto &cur_regmap = regmap_sn76489[chip_idx];

	cur_regmap.used = true;


	cur_regmap.is_latch = (data >> 7) & 0x1;

	if (cur_regmap.is_latch) {
		cur_regmap.channel = (data >> 5) & 0x3;
		cur_regmap.is_volume = (data >> 4) & 0x1;
		cur_regmap.data = data & 0xf;
	} else {
		cur_regmap.data = data & 0x3f;
	}

	if (cur_regmap.is_volume) {
		cur_regmap.att[cur_regmap.channel] = cur_regmap.data & 0xf;
	} else {
		if (cur_regmap.channel == 3) {
			cur_regmap.noise_ctrl = cur_regmap.data & 0x7;
		} else {
			if (cur_regmap.is_latch) {
				cur_regmap.freq[cur_regmap.channel] = (cur_regmap.freq[cur_regmap.channel] & ~0xful) | (cur_regmap.data & 0xf);
			} else {
				cur_regmap.freq[cur_regmap.channel] = (cur_regmap.freq[cur_regmap.channel] & 0xf) | ((cur_regmap.data << 4) & 0x3f0);
			}
		}
	}

//	uint8_t dat = retrowave_invert_byte(data);
//
//	if (dat & 0x1) {
//		cur_regmap.last_reg = (data >> 1) & 0x07;
//	}
//
//	uint8_t idx = 0, mode = 0; // 1: Freq, 2: Att
//
//	switch (cur_regmap.last_reg) {
//		case 0: // Tone 1 Freq, 000
//			mode = 1;
//			idx = 0;
//			break;
//		case 2: // Tone 2 Freq, 010
//			mode = 1;
//			idx = 1;
//			break;
//		case 1: // Tone 3 Freq, 100
//			mode = 1;
//			idx = 2;
//			break;
//		case 4: // Tone 1 Att
//			mode = 2;
//			idx = 0;
//			break;
//		case 6: // Tone 2 Att
//			mode = 2;
//			idx = 1;
//			break;
//		case 5: // Tone 3 Att
//			mode = 2;
//			idx = 2;
//			break;
//		case 3: // Noise ctrl, 110
//			cur_regmap.noise_ctrl = (data >> 4) & 0x0f;
//			break;
//		case 7: // Noise att
//			cur_regmap.noise_att = (data >> 4) & 0x0f;
//			break;
//	}
//
//	if (mode == 1) { // Freq
//		auto &val = cur_regmap.freq[idx];
//
//		if (dat & 0x1) { // reg + upper 4 bits
//			uint8_t val_upper = (data >> 4) & 0x0f; // Extract upper 4 bits
//			val &= 0x3f; // Preserve lower 6 bits
//			val |= val_upper << 6; // Merge
//		} else { // lower 6 bits
//			uint8_t val_lower = (data >> 2) & 0x3f; // Extract lower 6 bits
//			val &= 0x3c0; // Preserve upper 4 bits
//			val |= val_lower; // Merge
//		}
//	} else if (mode == 2) { // Att
//		if (dat & 0x1) {
//			cur_regmap.att[idx] = (data >> 4) & 0x0f;
//		}
//	}


}
