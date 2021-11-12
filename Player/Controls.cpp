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

void RetroWavePlayer::term_attr_disable_buffering() {
	struct termios oldt, newt;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~( ICANON | ECHO );
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

void RetroWavePlayer::term_attr_save() {
	tcgetattr(STDIN_FILENO, &term_state);
}

void RetroWavePlayer::term_attr_load() {
	tcsetattr(STDIN_FILENO, TCSANOW, static_cast<const termios *>(&term_state));
}

int RetroWavePlayer::term_read_char() {
	uint8_t buf;
	set_nonblocking(STDIN_FILENO);
	ssize_t rc = read(STDIN_FILENO, &buf, 1);
	set_nonblocking(STDIN_FILENO, false);

	if (rc == 1)
		return buf;
	else
		return -1;
}

void RetroWavePlayer::term_clear() {
	printf("\033[H");
	printf("\033[2J");
	printf("\033[3J");
}

void RetroWavePlayer::term_move_0_0() {
	printf("\033[H");
}

void RetroWavePlayer::controls_parse_key_commands() {
	int c = term_read_char();

	switch (c) {
		case 'q':
			key_command = QUIT;
			break;
		case 'p':
		case ' ':
			key_command = PAUSE_OR_PLAY;
			break;
		case '>':
		case '\n':
		case '.':
			key_command = NEXT;
			break;
		case '<':
		case ',':
			key_command = PREV;
			break;
		case 's':
			key_command = SINGLE_FRAME;
			break;
		case '/':
			key_command = FAST_FORWARD;
			break;
		default:
			key_command = NONE;
			break;
	}
}

void RetroWavePlayer::single_frame_hook() {
	if (!single_step)
		return;

	flush_chips();
	osd_show();
	puts("== Single frame mode ==");

	bool done = false;

	while (!done) {

		usleep(1000);
		controls_parse_key_commands();

		switch (key_command) {
			case QUIT:
				puts("Exiting...");
				do_exit(0);
				break;
			case PAUSE_OR_PLAY:
				puts("== Resume ==");
				single_step = false;
				done = true;
				term_clear();
				clock_gettime(RETROWAVE_PLAYER_TIME_REF, &sleep_end);
				break;
			case SINGLE_FRAME:
//				puts("== Next frame ==");
				done = true;
				single_step = true;
				break;
			default:
				break;
		}
	}
}

