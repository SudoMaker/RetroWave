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

//void RetroWavePlayer::char16_to_string(std::string &str, int16_t *c16) {
//	str.clear();
//
//	auto cd = iconv_open("UTF8", "UTF16LE");
//
//	assert(cd);
//
//	size_t len_in = (tinyvgm_strlen16(c16) + 1) * 2;
//	size_t len_out = len_in * 2;
//
//	char *outbuf = (char *)alloca(len_out);
//	memset(outbuf, 0, len_out);
//
//	char *inptr = (char *)c16;
//	char *outptr = outbuf;
//
//	iconv(cd, &inptr, &len_in, &outptr, &len_out);
//
//	str = outbuf;
//	iconv_close(cd);
//}

void RetroWavePlayer::char16_to_string(std::string &str, int16_t *c16) {
	str.clear();

	if (!c16)
		return;

	size_t c16_len = tinyvgm_strlen16(c16);

	if (!c16_len)
		return;

	std::u16string source = (char16_t *)c16;

	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> convert;
	str = convert.to_bytes(source);
//
//	size_t buflen_in = (c16_len + 1) * 2;
//
//
////	std::locale loc("C");
//	auto& f = std::use_facet<std::codecvt<char16_t, char, std::mbstate_t>>(std::locale());
//
//	std::mbstate_t mb{}; // initial shift state
//	str.resize(buflen_in * 2);
//
//	const char16_t* from_next;
//	char* to_next;
//
//	f.out(mb, reinterpret_cast<const char16_t *>(c16), reinterpret_cast<const char16_t *>(c16 + c16_len),
//	     from_next, &str[0], &str[str.size()], to_next);
//
//	str.resize(to_next - &str[0]);
}

void RetroWavePlayer::gd3_to_info(TinyVGMGd3Info *g) {
	char16_to_string(metadata.title, g->title);
	char16_to_string(metadata.album, g->album);
	char16_to_string(metadata.system_name, g->system_name);
	char16_to_string(metadata.composer, g->composer);
	char16_to_string(metadata.release_date, g->release_date);
	char16_to_string(metadata.converter, g->converter);
	char16_to_string(metadata.note, g->note);

	char16_to_string(metadata.title_jp, g->title_jp);
	char16_to_string(metadata.album_jp, g->album_jp);
	char16_to_string(metadata.system_name_jp, g->system_name_jp);
	char16_to_string(metadata.composer_jp, g->composer_jp);
}
