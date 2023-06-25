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
		v.resize((reg+1+15) & ~15); // round up to nearest 16, so that osd_show_regmaps() does not access invalid memory
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

}
