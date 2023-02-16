#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#define SDL_MAIN_HANDLED

#include <iostream>
#include <time.h>
#include <stdlib.h>
#include <chrono>
#include <thread>

#include "SDL.h"

uint8_t font[] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

uint8_t memory[4096]{};

uint8_t V[16]{}; // Registers
uint16_t I{}; // Index pointer
uint16_t pc = 0x200; // Program counter
uint16_t stack[16]{};
uint8_t sp{}; // Stack pointer
uint16_t opcode{};

uint8_t display[64 * 32]{};

uint8_t delay_timer{};
uint8_t sound_timer{};

uint8_t keypad[16]{}; // Keypad

bool drawFlag = false;

SDL_Window* window{};
SDL_Renderer* renderer{};
SDL_Texture* texture{};

// Sets up the SDL context
int start_sdl()
{
	SDL_SetMainReady();

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		fprintf(stderr, "SDL video init failed: %s\n", SDL_GetError());
		return -1;
	}

	window = SDL_CreateWindow("Chip-8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 16 * 64, 16 * 32, SDL_WINDOW_SHOWN);

	if (window == NULL)
	{
		fprintf(stderr, "Window could not be created: %s\n", SDL_GetError());
		return -1;
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	if (renderer == NULL)
	{
		printf("[FAIL] %s\n", SDL_GetError());
		return -1;
	}

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 64, 32);

	if (texture == NULL)
	{
		printf("[FAIL] %s\n", SDL_GetError());
		return -1;
	}

	return 0;
}

int load_rom()
{
	FILE* rom = fopen("roms/demos/Trip8 Demo (2008) [Revival Studios].ch8", "rb");
	if (!rom)
	{
		std::cout << "ERROR OPENING ROM FILE";
		return -1;
	}

	fseek(rom, 0, SEEK_END); // Get file size
	size_t size = ftell(rom);

	rewind(rom);
	// ROM is loaded into memory at 0x200
	fread(&memory[0x200], sizeof(char), size, rom);

	return 0;
}

// Updates window display by reading into the Chip-8's display buffer
void update_display()
{
	unsigned int d[64 * 32 * 4]{};

	for (int i = 0; i < sizeof(display); i++)
	{
		d[i] = display[i] * 255;
	}

	SDL_UpdateTexture(texture, NULL, d, 64 * 4);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

// Processes user input with the SDL library
void process_input()
{
	SDL_Event event;

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
					case SDLK_0:
						keypad[0x0] = 1;
						std::cout << "0" << std::endl;
						break;

					case SDLK_1:
						keypad[0x1] = 1;
						std::cout << "1" << std::endl;
						break;

					case SDLK_2:
						keypad[0x2] = 1;
						std::cout << "2" << std::endl;
						break;

					case SDLK_3:
						keypad[0x3] = 1;
						std::cout << "3" << std::endl;
						break;

					case SDLK_a:
						keypad[0x4] = 1;
						std::cout << "4" << std::endl;
						break;

					case SDLK_z:
						keypad[0x5] = 1;
						std::cout << "5" << std::endl;
						break;

					case SDLK_e:
						keypad[0x6] = 1;
						std::cout << "6" << std::endl;
						break;

					case SDLK_q:
						keypad[0x7] = 1;
						std::cout << "7" << std::endl;
						break;

					case SDLK_s:
						keypad[0x8] = 1;
						std::cout << "8" << std::endl;
						break;

					case SDLK_d:
						keypad[0x9] = 1;
						std::cout << "9" << std::endl;
						break;

					case SDLK_w:
						keypad[0xA] = 1;
						break;

					case SDLK_c:
						keypad[0xB] = 1;
						break;

					case SDLK_4:
						keypad[0xC] = 1;
						break;

					case SDLK_r:
						keypad[0xD] = 1;
						break;

					case SDLK_f:
						keypad[0xE] = 1;
						break;

					case SDLK_v:
						keypad[0xF] = 1;
						break;
				}
				break;

			case SDL_KEYUP:
				switch (event.key.keysym.sym)
				{
					case SDLK_0:
						keypad[0x0] = 0;
						break;

					case SDLK_1:
						keypad[0x1] = 0;
						break;

					case SDLK_2:
						keypad[0x2] = 0;
						break;

					case SDLK_3:
						keypad[0x3] = 0;
						break;

					case SDLK_a:
						keypad[0x4] = 0;
						break;

					case SDLK_z:
						keypad[0x5] = 0;
						break;

					case SDLK_e:
						keypad[0x6] = 0;
						break;

					case SDLK_q:
						keypad[0x7] = 0;
						break;

					case SDLK_s:
						keypad[0x8] = 0;
						break;

					case SDLK_d:
						keypad[0x9] = 0;
						break;

					case SDLK_w:
						keypad[0xA] = 0;
						break;

					case SDLK_c:
						keypad[0xB] = 0;
						break;

					case SDLK_4:
						keypad[0xC] = 0;
						break;

					case SDLK_r:
						keypad[0xD] = 0;
						break;

					case SDLK_f:
						keypad[0xE] = 0;
						break;

					case SDLK_v:
						keypad[0xF] = 0;
						break;
				}
				break;
		}
	}
}

// Dump memory to console
void dump_memory()
{
	for (unsigned int i = 0; i < 4096; i += 8)
	{
		std::cout << std::hex << i << " : ";
		std::cout << std::hex << (int)memory[i] << (int)memory[i + 1] << " ";
		std::cout << std::hex << (int)memory[i + 2] << (int)memory[i + 3] << " ";
		std::cout << std::hex << (int)memory[i + 4] << (int)memory[i + 5] << " ";
		std::cout << std::hex << (int)memory[i + 6] << (int)memory[i + 7] << std::endl;
	}
}

int main(int argc, char* argv[]) {
	if (start_sdl() != 0)
		return -1;

	// Write font in memory from 050-09F
	memcpy(&memory[0x50], font, sizeof(font));

	if (load_rom() != 0)
		return -1;

	// Seeds pseudo random generator
	srand((unsigned)time(NULL));

	// Emulation loop
	for (;;)
	{
		// Retrieve opcode from memory
		opcode = memory[pc] << 8 | memory[pc + 1];
		std::cout << "Opcode is ";
		std::cout << std::hex << (int)opcode << std::endl;

		// Switch over 4 first bits of the opcode
		switch (opcode & 0xF000)
		{
			case 0x0000:
				switch (opcode)
				{
					case 0x00E0: // 00E0 : Clears the screen
						memset(display, 0, sizeof(display));
						pc += 2;
						break;

					case 0x00EE: // 00EE : Returns from subroutine
						std::cout << "Returning from subroutine" << std::endl;
						sp--;
						pc = stack[sp];
						pc += 2;
						break;

					default:
						break;
				}
				break;

			case 0x1000: // 1NNN : Jumps to address NNN
				std::cout << "Jumping to address ";
				std::cout << std::hex << (opcode & 0xFFF) << std::endl;
				pc = (opcode & 0x0FFF);
				break;

			case 0x2000: // 2NNN : Calls subroutine at NNN
				std::cout << "Calling subroutine" << std::endl;
				stack[sp] = pc;
				sp++;
				pc = opcode & 0x0FFF;
				break;

			case 0x3000: // 3XNN : Skips next instruction if VX == NN
				if (V[((opcode & 0x0F00) >> 8)] == (opcode & 0x00FF))
				{
					pc += 4;
					std::cout << "Register equal to value, skipping next instruction" << std::endl;
				}
				else
				{
					pc += 2;
					std::cout << "Register not equal to value, not skipping next instruction" << std::endl;
				}

				break;

			case 0x4000: // 4XNN : Skips next instruction if VX != NN
				if (V[((opcode & 0x0F00) >> 8)] != (opcode & 0x00FF))
				{
					pc += 4;
				}
				else
				{
					pc += 2;
				}

				break;

			case 0x5000: // 5XY0 : Skips next instruction if VX == VY
				if (V[((opcode & 0x0F00) >> 8)] == V[(opcode & 0x00F0) >> 4])
				{
					pc += 4;
				}
				else
				{
					pc += 2;
				}
				break;

			case 0x6000: // 6XNN : Sets register VX to NN
				std::cout << "Setting register V";
				std::cout << std::hex << ((opcode & 0x0F00) >> 8);
				std::cout << " to value ";
				std::cout << std::hex << (opcode & 0x00FF) << std::endl;

				V[((opcode & 0x0F00) >> 8)] = (opcode & 0x00FF);
				pc += 2;
				break;

			case 0x7000: // 7XNN : Adds NN to VX
				std::cout << "Adding value ";
				std::cout << std::hex << (opcode & 0x00FF);
				std::cout << " to register ";
				std::cout << std::hex << ((opcode & 0x0F00) >> 8) << std::endl;

				V[((opcode & 0x0F00) >> 8)] += (opcode & 0x00FF);
				pc += 2;
				break;

			case 0x8000:
				switch (opcode & 0x000F)
				{
					case 0x0000: // 8XY0 : Sets VX to the value of VY
						V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
						pc += 2;
						break;

					case 0x0001: // 8XY1 : Sets VX to VX OR VY (VX | VY)
						V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
						pc += 2;
						break;

					case 0x0002: // 8XY2 : Sets VX to VX AND XY (VX & VY)
						V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
						pc += 2;
						break;

					case 0x0003: // 8XY3 : Sets VX to VX XOR VY (VX ^ VY)
						V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
						pc += 2;
						break;

					case 0x0004: // 8XY4 : Adds VY to VX
					{
						uint8_t vx = (opcode & 0x0F00) >> 8;
						uint8_t vy = (opcode & 0x00F0) >> 4;

						uint16_t sum = V[vx] + V[vy];

						if (sum > 255)
							V[0xF] = 1; // Carry
						else
							V[0xF] = 0; // No carry

						V[(opcode & 0x0F00) >> 8] = sum & 0xFF;

						pc += 2;
						break;
					}

					case 0x0005: // 8XY5 : Substracts VY from VX
					{
						uint8_t vx = (opcode & 0x0F00) >> 8;
						uint8_t vy = (opcode & 0x00F0) >> 4;

						if (V[vx] > V[vy])
						{
							V[0xF] = 1;
						}
						else
						{
							V[0xF] = 0;
						}

						V[vx] -= V[vy];

						pc += 2;
						break;
					}

					case 0x0006: // 8XY6 : Stores the least significant bit of VX in VF and then shifts VX to the right by 1
						V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x01;
						V[(opcode & 0x0F00) >> 8] >>= 1;
						pc += 2;
						break;

					case 0x0007: // 8XY7 : VX is set to VY - VX
					{
						uint8_t vx = (opcode & 0x0F00) >> 8;
						uint8_t vy = (opcode & 0x00F0) >> 4;

						if (V[vy] > V[vx])
						{
							V[0xF] = 1;
						}
						else
						{
							V[0xF] = 0;
						}

						V[vx] = V[vy] - V[vx];

						pc += 2;
						break;
					}

					case 0x000E: // 8XYE : Stores the most significant bit of VX in VF and then shifts VX to the left by 1
						V[0xF] = V[(opcode & 0x0F00) >> 8] >> 7;
						V[(opcode & 0x0F00) >> 8] <<= 1;
						pc += 2;
						break;
				}
				break;

			case 0x9000: // 9XY0 : Skips next instruction if VX != VY
				if (V[((opcode & 0x0F00) >> 8)] != V[(opcode & 0x00F0) >> 4])
				{
					pc += 4;
				}
				else
				{
					pc += 2;
				}
				break;

			case 0xA000: // ANNN : Sets I register to the address NNN
				I = (opcode & 0x0FFF);
				std::cout << "Setting I to value ";
				std::cout << std::hex << (opcode & 0x0FFF) << std::endl;
				pc += 2;
				break;

			case 0xB000: // BNNN : Jumps to the address NNN + V0
				pc = (opcode & 0x0FFF) + V[0];
				break;

			case 0xC000: // Sets VX to the result of a bitwise AND between a random number and NN
			{
				V[((opcode & 0x0F00) >> 8)] = (rand() % 256) & (opcode & 0x00FF);
				pc += 2;
				break;
			}

			case 0xD000: // DXYN : Draws a sprite with coordinates in registers VX and VY of 8 pixels width and N pixels height
			{
				uint16_t x = V[(opcode & 0x0F00) >> 8] % 64;
				uint16_t y = V[(opcode & 0x00F0) >> 4] % 32;
				uint16_t height = opcode & 0x000F;
				uint16_t pixel;

				std::cout << "Drawing at coords (x, y) ";
				std::cout << std::hex << x;
				std::cout << " ";
				std::cout << std::hex << y;
				std::cout << " with height ";
				std::cout << std::hex << height << std::endl;

				V[0xF] = 0; // VF register is used to register collisions
				for (int yline = 0; yline < height; yline++)
				{
					pixel = memory[I + yline];
					for (int xline = 0; xline < 8; xline++)
					{
						if ((pixel & (0x80 >> xline)) != 0)
						{
							if (display[x + xline + ((yline + y) * 64)] == 1) // If the pixel is already set on display, register collision in VF
								V[0xF] = 1;
							display[x + xline + ((yline + y) * 64)] ^= 1;
						}
					}
				}
				
				drawFlag = true;
				pc += 2;

				break;
			}

			case 0xE000:
				switch (opcode & 0x0FF)
				{
					case 0x009E: // EX9E : If the key in VX is pressed, skips next instruction
						if (keypad[V[((opcode & 0x0F00) >> 8)]] == 1)
						{
							pc += 4;
						}
						else
						{
							pc += 2;
						}
						break;

					case 0x00A1: // EXA1 : If the key in VX is not pressed, skips next instruction
						if (keypad[V[((opcode & 0x0F00) >> 8)]] == 0)
						{
							pc += 4;
						}
						else
						{
							pc += 2;
						}
						break;
				}
				break;

			case 0xF000:
				switch (opcode & 0x00FF)
				{
					case 0x0007: // FX07 : Sets VX to the value of the delay timer
						V[((opcode & 0x0F00) >> 8)] = delay_timer;
						pc += 2;
						break;

					case 0x000A: // FX0A : Key press is awaited, then stored in VX (halts until next key event)
					{
						bool key_pressed = false;

						for (int i = 0; i < 16; i++)
						{
							if (keypad[i])
							{
								V[((opcode & 0x0F00) >> 8)] = i;
								key_pressed = true;
							}
						}

						if (key_pressed)
						{
							pc += 2;
						}

						break;
					}

					case 0x0015: // FX15 : Sets delay timer to VX
						delay_timer = V[((opcode & 0x0F00) >> 8)];
						pc += 2;
						break;

					case 0x0018: // FX18 : Sets sound timer to VX
						sound_timer = V[((opcode & 0x0F00) >> 8)];
						pc += 2;
						break;

					case 0x001E: // FX1E : Adds VX to I, VF unaffected
						I += V[((opcode & 0x0F00) >> 8)];
						pc += 2;
						break;
					
					case 0x0029: // FX29 : Sets I to the location of the sprite for the character in VX
						I = 0x50 + (V[((opcode & 0x0F00) >> 8)] * 5);
						pc += 2;
						break;

					case 0x0033: // FX33 : Stores BCD representation
					{
						uint8_t vx = V[((opcode & 0x0F00) >> 8)];

						memory[I] = vx / 100;
						memory[I + 1] = (vx / 10) % 10;
						memory[I + 2] = (vx % 100) % 10;
						pc += 2;

						break;
					}

					case 0x0055: // FX55 : Stores values of registers V0-VX in memory, starting at I
						for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++)
						{
							memory[I + i] = V[i];
						}
						pc += 2;
						break;

					case 0x0065: // FX65 : Fill registers V0-VX with values from memory, starting at I
						for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++)
						{
							V[i] = memory[I + i];
						}
						pc += 2;
						break;
				}
				break;

		}

		process_input();

		// If display buffer has been updated, update the display
		if (drawFlag)
		{
			update_display();
			drawFlag = false;
		}

		if (delay_timer > 0)
			delay_timer--;

		if (sound_timer > 0)
			sound_timer--;

		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}

	return 1;
}