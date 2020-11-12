#include "tokenizer.h"

#include "scanner.h"
TKType findKeyword(char* start, char* current){
	int length = current - start;
	switch(start[0]){
		case 'a':{
			if(length > 1){
				switch(start[1]){
					case 'n':{
						if(length > 2){
							switch(start[2]){
								case 'd': return TK_AND;
								case 'i': return checkKeyword(3, 1, "m", TK_ANIM);
							}
						}
					} break;

					case 's':{
						if(length > 2){
							switch(start[2]){

								case 'i':{
									if(length > 3 && start[3] == 'n'){
										return TK_ASIN;
									}
								} break;

							}
						}else{
							return TK_AS;
						}
					} break;

					case 'c': return checkKeyword(2, 2, "os", TK_ACOS);
					case 't':{
						if(length > 2){
							switch(start[2]){

								case 'a':{
									if(length > 3 && start[3] == 'n'){
										return TK_ATAN;
									}
								} break;

							}
						}else{
							return TK_AT;
						}
					} break;

					case 'r': return checkKeyword(2, 1, "c", TK_ARC);
				}
			}
		return TK_ID;
		} break;

		case 'f':{
			if(length > 1){
				switch(start[1]){
					case 'a': return checkKeyword(2, 3, "lse", TK_FALSE);
					case 'o': return checkKeyword(2, 1, "r", TK_FOR);
					case 'u': return checkKeyword(2, 2, "nc", TK_FUNC);
					case 'r': return checkKeyword(2, 2, "om", TK_FROM);
				}
			}
		return TK_ID;
		} break;

		case 't':{
			if(length > 1){
				switch(start[1]){
					case 'r': return checkKeyword(2, 2, "ue", TK_TRUE);
					case 'a': return checkKeyword(2, 1, "n", TK_TAN);
					case 'e': return checkKeyword(2, 2, "xt", TK_TEXT);
					case 'o': return TK_TO;
					case 'u': return checkKeyword(2, 2, "rn", TK_TURN);
				}
			}
		return TK_ID;
		} break;

		case 'n': return checkKeyword(1, 3, "ull", TK_NULL);
		case 'b': return checkKeyword(1, 1, "y", TK_BY);
		case 'd':{
			if(length > 1){
				switch(start[1]){
					case 'o': return TK_DO;
					case 'r': return checkKeyword(2, 2, "aw", TK_DRAW);
					case 'e': return checkKeyword(2, 1, "f", TK_DEF);
				}
			}
		return TK_ID;
		} break;

		case 'i':{
			if(length > 1){
				switch(start[1]){
					case 'f': return TK_IF;
					case 'n': return TK_IN;
				}
			}
		return TK_ID;
		} break;

		case 'l':{
			if(length > 1){
				switch(start[1]){
					case 'e': return checkKeyword(2, 1, "t", TK_LET);
					case 'i': return checkKeyword(2, 2, "ne", TK_LINE);
				}
			}
		return TK_ID;
		} break;

		case 'w':{
			if(length > 1){
				switch(start[1]){
					case 'h': return checkKeyword(2, 3, "ile", TK_WHILE);
					case 'i': return checkKeyword(2, 2, "th", TK_WITH);
				}
			}
		return TK_ID;
		} break;

		case 'r':{
			if(length > 1){
				switch(start[1]){
					case 'e':{
						if(length > 2){
							switch(start[2]){
								case 'c': return checkKeyword(3, 1, "t", TK_RECT);
								case 't': return checkKeyword(3, 3, "urn", TK_RETURN);
								case 'p': return checkKeyword(3, 3, "eat", TK_REPEAT);
							}
						}
					} break;

					case 'a':{
						if(length > 2){
							switch(start[2]){
								case 'd': return checkKeyword(3, 4, "ians", TK_RADIANS);
								case 'n': return checkKeyword(3, 1, "d", TK_RAND);
							}
						}
					} break;

				}
			}
		return TK_ID;
		} break;

		case 'c':{
			if(length > 1){
				switch(start[1]){
					case 'i': return checkKeyword(2, 4, "rcle", TK_CIRCLE);
					case 'o': return checkKeyword(2, 1, "s", TK_COS);
					case 'b': return checkKeyword(2, 5, "ezier", TK_CBEZIER);
				}
			}
		return TK_ID;
		} break;

		case 's':{
			if(length > 1){
				switch(start[1]){
					case 'i': return checkKeyword(2, 1, "n", TK_SIN);
					case 'q': return checkKeyword(2, 2, "rt", TK_SQRT);
				}
			}
		return TK_ID;
		} break;

		case 'o': return checkKeyword(1, 1, "r", TK_OR);
		case 'p':{
			if(length > 1){
				switch(start[1]){
					case 'r': return checkKeyword(2, 3, "int", TK_PRINT);
					case 'a': return checkKeyword(2, 2, "th", TK_PATH);
					case 'o':{
						if(length > 2){
							switch(start[2]){
								case 'l':{
									if(length > 3){
										switch(start[3]){
											case 'y':{
												if(length > 4){
													switch(start[4]){
														case 'l': return checkKeyword(5, 3, "ine", TK_POLYLINE);
														case 'g': return checkKeyword(5, 2, "on", TK_POLYGON);
													}
												}
											} break;

										}
									}
								} break;

							}
						}
					} break;

				}
			}
		return TK_ID;
		} break;

		case 'e':{
			if(length > 1){
				switch(start[1]){
					case 'l':{
						if(length > 2){
							switch(start[2]){
								case 's': return checkKeyword(3, 1, "e", TK_ELSE);
								case 'l': return checkKeyword(3, 4, "ipse", TK_ELLIPSE);
							}
						}
					} break;

				}
			}
		return TK_ID;
		} break;

		case 'v':{
			if(length > 1){
				switch(start[1]){
					case 'e': return checkKeyword(2, 4, "rtex", TK_VERTEX);
					case 'a': return checkKeyword(2, 1, "r", TK_VAR);
					case 'i': return checkKeyword(2, 1, "a", TK_VIA);
				}
			}
		return TK_ID;
		} break;

		case 'h':{
			if(length > 1){
				switch(start[1]){
					case 's': return checkKeyword(2, 2, "in", TK_HSIN);
					case 'c': return checkKeyword(2, 2, "os", TK_HCOS);
					case 't': return checkKeyword(2, 2, "an", TK_HTAN);
				}
			}
		return TK_ID;
		} break;

		case 'm':{
			if(length > 1){
				switch(start[1]){
					case 'i': return checkKeyword(2, 4, "rror", TK_MIRROR);
					case 'o': return checkKeyword(2, 2, "ve", TK_MOVE);
				}
			}
		return TK_ID;
		} break;

		case 'j': return checkKeyword(1, 3, "ump", TK_JUMP);
		case 'q': return checkKeyword(1, 6, "bezier", TK_QBEZIER);
		case 'u': return checkKeyword(1, 4, "ngon", TK_UNGON);
		default: return TK_ID;
	}
}
CSType findConstant(char* start, char* current){
	int length = current - start;
	switch(start[0]){
		case 'c': return checkConstant(1, 5, "enter", CS_CENTER);
		case 'p':{
			if(length > 1){
				switch(start[1]){
					case 'i':{
						if(length > 2){
							switch(start[2]){

								case 'n':{
									if(length > 3 && start[3] == 'k'){
										return CS_PINK;
									}
								} break;

							}
						}else{
							return CS_PI;
						}
					} break;

					case 'u': return checkConstant(2, 4, "rple", CS_PURPLE);
				}
			}
		return CS_ERROR;
		} break;

		case 't':{
			if(length > 1){
				switch(start[1]){
					case 'a': return checkConstant(2, 1, "u", CS_TAU);
					case 'u': return checkConstant(2, 7, "rquoise", CS_TURQUOISE);
					case 'e': return checkConstant(2, 2, "al", CS_TEAL);
					case 'r': return checkConstant(2, 9, "ansparent", CS_TRANSPARENT);
				}
			}
		return CS_ERROR;
		} break;

		case 'e': return CS_E;
		case 'r': return checkConstant(1, 2, "ed", CS_RED);
		case 'o':{
			if(length > 1){
				switch(start[1]){
					case 'r': return checkConstant(2, 4, "ange", CS_ORANGE);
					case 'l': return checkConstant(2, 3, "ive", CS_OLIVE);
				}
			}
		return CS_ERROR;
		} break;

		case 'y':{
			if(length > 1){
				switch(start[1]){

					case 'e':{
						if(length > 2 && start[2] == 'l'){
							return checkConstant(3, 3, "low", CS_YELLOW);
						}
					} break;

				}
			}else{
				return CS_Y;
			}
		return CS_ERROR;
		} break;

		case 'b':{
			if(length > 1){
				switch(start[1]){
					case 'l':{
						if(length > 2){
							switch(start[2]){
								case 'u': return checkConstant(3, 1, "e", CS_BLUE);
								case 'a': return checkConstant(3, 2, "ck", CS_BLACK);
							}
						}
					} break;

					case 'r': return checkConstant(2, 3, "own", CS_BROWN);
				}
			}
		return CS_ERROR;
		} break;

		case 'm':{
			if(length > 1){
				switch(start[1]){
					case 'a':{
						if(length > 2){
							switch(start[2]){
								case 'r': return checkConstant(3, 3, "oon", CS_MAROON);
								case 'g': return checkConstant(3, 4, "enta", CS_MAGENTA);
							}
						}
					} break;

				}
			}
		return CS_ERROR;
		} break;

		case 'n': return checkConstant(1, 3, "avy", CS_NAVY);
		case 'a': return checkConstant(1, 3, "qua", CS_AQUA);
		case 's': return checkConstant(1, 5, "ilver", CS_SILVER);
		case 'l':{
			if(length > 1){
				switch(start[1]){
					case 'c': return checkConstant(2, 5, "orner", CS_LCORNER);
					case 'i': return checkConstant(2, 2, "me", CS_LIME);
				}
			}
		return CS_ERROR;
		} break;

		case 'i': return checkConstant(1, 5, "ndigo", CS_INDIGO);
		case 'v': return checkConstant(1, 5, "iolet", CS_VIOLET);
		case 'w': return checkConstant(1, 4, "hite", CS_WHITE);
		case 'g':{
			if(length > 1){
				switch(start[1]){
					case 'r':{
						if(length > 2){
							switch(start[2]){
								case 'e':{
									if(length > 3){
										switch(start[3]){
											case 'e': return checkConstant(4, 1, "n", CS_GREEN);
											case 'y': return CS_GREY;
										}
									}
								} break;

								case 'a': return checkConstant(3, 1, "y", CS_GRAY);
							}
						}
					} break;

				}
			}
		return CS_ERROR;
		} break;

		case 'x':{
			if(length > 1){
				switch(start[1]){

					case 'y':{
						return CS_XY;
					} break;

				}
			}else{
				return CS_X;
			}
		return CS_ERROR;
		} break;

		default: return CS_ERROR;
	}
}
