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

