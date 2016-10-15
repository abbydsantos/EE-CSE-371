/*
Jackson Cannon
CSE 371
Lab 5
checkers.c
 */
#include "sys/alt_stdio.h"
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>

#define WHITEPLAYER 1
#define WHITETURN 2
#define GAMEON 4

#define OCCUPIED 1
#define WHITE 2
#define KING 4

#define SIGNALBYTE 8

#define sendByte ((volatile char*)0x00021070)
#define recieveByte ((volatile char*)0x00021040)
#define startSend ((volatile char*)0x00021080)

#define whitePiecesLeft ((volatile char*)0x00021010)
#define blackPiecesLeft ((volatile char*)0x00021000)

#define NONE -1

int read_int() {
	int i = 0;
	int ch;
	while ((ch = alt_getchar()) != '\n') {
		if (ch > '9' || ch < '0')
			continue;
		i *= 10;
		i += ch - '0';
	}
	return i;
}

char read_char() {
	char c;
	do {
		c = alt_getchar();
	} while(c == '\n');
	return c;
}

void setup_board(char* board) {
	int i;
	for(i = 0; i<32; i++) {
		if(i<12) {
			board[i] = OCCUPIED;
		} else if(i<20) {
			board[i] = 0;
		} else {
			board[i] = OCCUPIED | WHITE;
		}
	}
}

void print_board(char* board, char state, char must_jump) {
  void print_piece(char piece) {
    printf("|");
    if(piece & OCCUPIED) {
      printf("(");
      if(piece & WHITE) {
        printf("█");
      } else {
        printf("_");
      }
      printf(")");
      if(piece & KING) {
        printf(")");
      } else {
        printf(" ");
      }
    } else {
      printf("    ");
    }
  }

  char numbers[32];
  if (state & WHITEPLAYER) {
    for (int i = 0; i < 32; i++) {
      numbers[i] = i;
    }
  } else {
    char tboard[32];
    for (int i = 0; i < 32; i++) {
      tboard[i] = board[31-i];
      numbers[i] = 31-i;
    }
    board = tboard;
  }

  void print_num(int num) {
    printf("|  ");
    if (num < 10) {
      printf(" ");
    }
    printf("%d", num);
  }

  void doline(char *board, char *numbers, int start) {
    printf("+----+----+----+----+----+----+----+----+\n");
    char sm4 = !(start % 8);

    if (sm4) {
      printf("|    ");
    }
    print_piece(board[start]);
    printf("|    ");
    print_piece(board[start+1]);
    printf("|    ");
    print_piece(board[start+2]);
    printf("|    ");
    print_piece(board[start+3]);

    if (!sm4) {
      printf("|    ");
    }
    printf("|\n");


    if (sm4) {
      printf("|    ");
    }
    print_num(numbers[start]+1);
    printf("|    ");
    print_num(numbers[start+1]+1);
    printf("|    ");
    print_num(numbers[start+2]+1);
    printf("|    ");
    print_num(numbers[start+3]+1);

    if (!sm4) {
      printf("|    ");
    }
    printf("|\n");

  }

  for(int i = 0; i<32; i+=4) {
    doline(board,numbers,i);
  }
  printf("+----+----+----+----+----+----+----+----+\n");
  if (state & WHITETURN) {
    printf("White's turn");
  } else {
    printf("Black's turn");
  }
  if (must_jump) {
    printf(" (must jump)");
  }
  printf(" |");

	char w=0;
	char b=0;

	//update piece counters
	for(i = 0; i<32; i++) {
	    if (board[i] & OCCUPIED) {
	        if (board[i] & WHITE) {
	            w++;
	        } else {
	            b++;
	        }
	    }
	}

	*whitePiecesLeft = w;
	*blackPiecesLeft = b;
}

// 0=up left 1=up right 2=down left 3=down right
int get_adjacent(int pos, int dir) {
	if(pos == NONE) {
		return NONE;
	}
	if(((dir & 1) == 0) && ((pos - 4) % 8 == 0)) {
		return NONE;
	}
	if(((dir & 1) == 1) && ((pos - 3) % 8 == 0)) {
		return NONE;
	}
	if(((dir & 2) == 0) && (pos < 4)) {
		return NONE;
	}
	if(((dir & 2) == 2) && (pos > 27)) {
		return NONE;
	}
	int oddrow = !!(pos & 4);
	int found = NONE;
	switch (dir) {
	case 0:
		found = pos-(4+oddrow);
		break;
	case 1:
		found = pos-(3+oddrow);
		break;
	case 2:
		found = pos+(4-oddrow);
		break;
	case 3:
		found = pos+(5-oddrow);
		break;
	}
	if (found < 0 || found > 31) {
		found = NONE;
	}
	return found;
}

int adjacent_dir(int s1, int s2) {
	int i;
	for(i=0; i<4; i++) {
		if (get_adjacent(s1, i) == s2) {
			return i;
		}
	}
	return NONE;
}

int jump_dir(int s1, int s2) {
	int i;
	for(i=0; i<4; i++) {
		if (get_adjacent(get_adjacent(s1, i), i) == s2) {
			return i;
		}
	}
	return NONE;
}

char valid_dir(char piece, int dir) {
	if (dir == NONE) {
		return 0;
	}
	if (!(piece & OCCUPIED)) {
		return 0;
	}
	if (piece & KING) {
		return 1;
	}
	return !(piece & WHITE) == !!(dir & 2);
}

//Returns 1 if piece can jump anywhere
char find_jump(char *board, int piece) {
	int i;
	for(i=0; i<4; i++) {
		if (valid_dir(board[piece], i)) {
			int inbetween = get_adjacent(piece, i);
			int jumpto = get_adjacent(inbetween, i);
			if( (inbetween != NONE) &&
					(jumpto != NONE) &&
					(!(board[jumpto] & OCCUPIED)) &&
					(board[inbetween] & OCCUPIED) &&
					(board[piece] & WHITE) != (board[inbetween] & WHITE) ) {
				return 1;
			}
		}
	}
	return 0;
}

//Returns 1 if piece can move anywhere
char find_move(char *board, int piece) {
	int i;
	for(i=0; i<4; i++) {
		if (valid_dir(board[piece], i)) {
			int adj = get_adjacent(piece, i);
			if( (adj != NONE) && (!(board[adj] & OCCUPIED)) ) {
				return 1;
			}
		}
	}
	return find_jump(board, piece);
}

char valid_move(char *board, int piece, int move, char must_jump) {
	int dir = NONE;
	if (!must_jump) {
		dir = adjacent_dir(piece, move);
		if (valid_dir(board[piece], dir)) {
			return !(board[move] & OCCUPIED);
		}
	}
	dir = jump_dir(piece, move);
	if (valid_dir(board[piece], dir)) {
		if(board[move] & OCCUPIED) {
			return 0;
		}
		int inbetween = get_adjacent(piece, dir);
		if(!(board[inbetween] & OCCUPIED)) {
			return 0;
		}
		return (board[piece] & WHITE) != (board[inbetween] & WHITE);
	}
	return 0;
}

char do_move(char *board, int piece, int move) {
	char jumped = 0;
	int dir = adjacent_dir(piece, move);
	if(dir != NONE) {
		board[move] = board[piece];
		board[piece] = 0;
	} else {
		//Assume jump
		dir = jump_dir(piece, move);
		int inbetween = get_adjacent(piece, dir);
		/* if(inbetween == NONE) {
      printf("ERROR");
    } */
		board[move] = board[piece];
		board[inbetween] = 0;
		board[piece] = 0;
		jumped = 1;
	}
	if (board[move] & WHITE) {
		if (move < 4) {
			board[move] |= KING;
		}
	} else {
		if (move > 27) {
			board[move] |= KING;
		}
	}
	return jumped & find_jump(board, move);
}

void send_byte(char byt) {
	//alt_printf("sent = %c\n", byt+'0');
	*sendByte = byt;
	*startSend = 1;
	usleep(10);
	*startSend = 0;
	usleep(1990);
}

void send_state(char *state, char *board) {
	send_byte(SIGNALBYTE);
	send_byte((*state) ^ WHITEPLAYER);
	int i;
	for (i = 0; i < 32; i++) {
		send_byte(board[i]);
	}
	send_byte(9);
}

void recieve_state(char *state, char *board) {
	while ((*recieveByte) != SIGNALBYTE) {

		//usleep(100000);
		//alt_printf("digit = %c\n", (recieveByte[0]+'0'));
	}
	// Wait so we're right in the middle of transmission
	usleep(1000);

	usleep(2000);
	*state = *recieveByte;
	int i;
	for (i = 0; i < 32; i++) {
		usleep(2000);
		board[i] = *recieveByte;
	}
	// Wait 2000 for zeroing byte
	usleep(2000);
}

char do_move_and_send(char *board, int piece, int move, char* state) {
	char out = do_move(board,piece,move);
	if (!out) {
		*state = (*state) ^ WHITETURN;
		print_board(board, *state, 0);
	}

	send_state(state, board);

	return out;
}

int main(int argc, char *argv[])
{
	*whitePiecesLeft = 0;
	*blackPiecesLeft = 0;

	char state;
	char board[32];
	printf("Welcome to checkers!\n");
	printf("Hook up the boards then Choose Team on the Black player first.\n");
	printf("Choose Team (b for Black, w for white): ");
	state = read_char();

	if (state == 'w' | state == 'W') {
		state = WHITEPLAYER | WHITETURN | GAMEON;
		setup_board(board);
		send_state(&state, board);
	} else {
		state = WHITETURN | GAMEON;
	}

	while(1) {
		// If it's our turn
		if ((!(state & WHITEPLAYER)) == (!(state & WHITETURN))) {
			int piece = NONE;
			int forcepiece = NONE;
			int move = NONE;

			char can_move = 0;
			char must_jump = 0;
			int i;
			for(i = 0; i < 32; i++) {
				if(((!(board[i] & WHITE)) == (!(state & WHITETURN))) && (board[i] & OCCUPIED)) {
					can_move |= find_move(board, i);
					must_jump |= find_jump(board, i);
				}
			}

			if (!can_move) {
				printf("\n You Lose! \n");
				state = 0;
				send_state(&state, board);
				while(1) {
				}
			}

			do {
				move = NONE;
				do {
					if (forcepiece == NONE) {
						do {
							print_board(board, state, must_jump);
							if (piece != NONE) {
								printf(" Invalid choice!.. ");
							}
							printf(" Choose piece: ");
							piece = read_int() - 1;
						} while(((!(board[piece] & WHITE)) != (!(state & WHITETURN))) | (!(board[piece] & OCCUPIED)));
					} else {
						piece = forcepiece;
					}
					print_board(board, state, must_jump);
					if (move != NONE) {
						printf(" Invalid.. ");
					}
					printf(" Piece: %d Choose move: ", piece+1);
					move = read_int() - 1;
				} while(!valid_move(board, piece, move, must_jump));
				forcepiece = move;
			} while(do_move_and_send(board,piece,move,&state));
		} else {
			//Not our turn
			printf("\nWaiting for opponent...\n");
			recieve_state(&state, board);
			if (state == 0) {
				printf("\n You Win! \n");
				while(1) {
				}
			}
			//If it's still not our turn, print the intermediate board
			if ((!(state & WHITEPLAYER)) != (!(state & WHITETURN))) {
				print_board(board, state, 0);
			}
		}
	}
}
