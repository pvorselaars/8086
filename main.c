#include <stdio.h>
#include <stdint.h>

#define OP_MOV_REG_TO_REG 0b10001000
#define OP_MOV_IMMEDIATE  0b10110000
#define OP_MOV_IMM_TO_RM  0b11000110
#define OP_MOV_MEM_TO_ACC 0b10100000
#define OP_MOV_SEG_TO_RM  0b10001100

static char *registers[] = {
	"al",
	"cl",
	"dl",
	"bl",
	"ah",
	"ch",
	"dh",
	"bh",
	"ax",
	"cx",
	"dx",
	"bx",
	"sp",
	"bp",
	"si",
	"di"
};

static char *segment_registers[] = {
	"es",
	"cs",
	"ss",
	"ds"
};

static char *ea_base[] = {
	"bx + si",
	"bx + di",
	"bp + si",
	"bp + di",
	"si",
	"di",
	"bp",
	"bx"
};

void format_displacement(char *ea, int16_t disp, int16_t rm)
{
	if (disp == 0) {
		sprintf(ea, "[%s]", ea_base[rm]);
	} else if (disp > 0) {
		sprintf(ea, "[%s + %d]", ea_base[rm], disp);
	} else {
		sprintf(ea, "[%s - %d]", ea_base[rm], -disp);
	}
}

void get_8bit_displacement(char *ea, int16_t rm, FILE *input)
{
	int disp_byte = fgetc(input);
	if (disp_byte == EOF) {
		fprintf(stderr, "Unexpected EOF reading displacement\n");
		return;
	}
	int16_t disp = (int8_t) disp_byte;

	format_displacement(ea, disp, rm);
}

void get_16bit_displacement(char *ea, int16_t rm, FILE *input)
{
	int lo = fgetc(input);
	int hi = fgetc(input);
	if (lo == EOF || hi == EOF) {
		fprintf(stderr, "Unexpected EOF reading displacement\n");
		return;
	}
	int16_t disp = (int16_t) (lo | (hi << 8));

	format_displacement(ea, disp, rm);
}

int16_t get_immediate(FILE *input, int16_t w) {
			int16_t data;
			int lo = fgetc(input);
			if (lo == EOF) {
				fprintf(stderr, "Unexpected EOF reading immediate\n");
				return 0;
			}

			if (w) {
				int hi = fgetc(input);
				if (hi == EOF) {
					fprintf(stderr, "Unexpected EOF reading immediate\n");
					return 0;
				}
				data = lo | (hi << 8);
			} else {
				data = lo;
			}
			return data;
}

int main(int argc, char *argv[])
{

	if (argc < 2)
		return -1;

	FILE *input = fopen(argv[1], "r");

	if (!input) {
		perror("Error opening file");
		return -1;
	}

	printf("; %s\nbits 16\n", argv[1]);

	int32_t c;
	int16_t data, d, w, mod, reg, rm, sr;

	while ((c = fgetc(input)) != EOF) {
		char ea[64];	// effective address

		// MOV register/memory to/from register
		if ((c & 0b11111100) == OP_MOV_REG_TO_REG) {
			d = (c >> 1) & 1;
			w = c & 1;

			c = fgetc(input);
			if (c == EOF) {
				fprintf(stderr, "Unexpected EOF after opcode\n");
				fclose(input);
				return -1;
			}

			mod = (c >> 6);
			reg = (c >> 3) & 7;
			rm = c & 7;

			char *reg_name = registers[(w << 3) + reg];

			switch (mod) {
			case 0b11:	// register to register
				char *rm_name = registers[(w << 3) + rm];
				char *src = d ? rm_name : reg_name;
				char *dst = d ? reg_name : rm_name;
				printf("mov %s, %s\n", dst, src);
				break;

			case 0b01:	// memory mode, 8 bit displacement
				get_8bit_displacement(ea, rm, input);
				printf("mov %s, %s\n", d ? reg_name : ea, d ? ea : reg_name);
				break;

			case 0b10:	// memory mode, 16 bit displacement
				get_16bit_displacement(ea, rm, input);
				printf("mov %s, %s\n", d ? reg_name : ea, d ? ea : reg_name);
				break;

			case 0b00:	// memory mode, no displacement
				if (rm == 0b110) {
					int lo = fgetc(input);
					int hi = fgetc(input);
					if (lo == EOF || hi == EOF) {
						fprintf(stderr, "Unexpected EOF reading address\n");
						fclose(input);
						return -1;
					}
					uint16_t addr = lo | (hi << 8);
					sprintf(ea, "[%d]", addr);
				} else {
					sprintf(ea, "[%s]", ea_base[rm]);
				}
				printf("mov %s, %s\n", d ? reg_name : ea, d ? ea : reg_name);
				break;

			default:
				fprintf(stderr, "Unsupported addressing mode: %b\n", mod);
				return -1;
			}

			continue;
		}
		// MOV immediate to register
		if ((c & 0b11110000) == OP_MOV_IMMEDIATE) {
			w = (c >> 3) & 1;
			reg = c & 7;

			data = get_immediate(input, w);

			printf("mov %s, %d\n", registers[(w << 3) + reg], data);

			continue;
		}
		// MOV immediate to register/memory
		if ((c & 0b11111110) == OP_MOV_IMM_TO_RM) {
			w = c & 1;

			c = fgetc(input);
			if (c == EOF) {
				fprintf(stderr, "Unexpected EOF after opcode\n");
				fclose(input);
				return -1;
			}

			mod = (c >> 6);
			reg = (c >> 3) & 7;
			rm = c & 7;

			switch (mod) {
			case 0b01:	// memory mode, 8 bit displacement
				get_8bit_displacement(ea, rm, input);
				data = get_immediate(input, w);
				printf("mov %s, %s%d\n", ea, w ? "word " : "byte ", data);
				break;

			case 0b10:	// memory mode, 16 bit displacement
				get_16bit_displacement(ea, rm, input);
				data = get_immediate(input, w);
				printf("mov %s, %s%d\n", ea, w ? "word " : "byte ", data);
				break;

			case 0b00:	// memory mode, no displacement
				if (rm == 0b110) {
					uint16_t addr = get_immediate(input, w);
					sprintf(ea, "[%d]", addr);
				} else {
					data = get_immediate(input, w);
					sprintf(ea, "[%s]", ea_base[rm]);
				}
				printf("mov %s, %s%d\n", ea, w ? "word " : "byte ", data);
				break;

			default:
				fprintf(stderr, "Unsupported addressing mode: %b\n", mod);
				return -1;
			}

			continue;
		}
		// MOV memory to accumalator
		if ((c & 0b11111100) == OP_MOV_MEM_TO_ACC) {
			d = (c >> 1) & 1;

			data = get_immediate(input, 1);

			if (d) {
				printf("mov [%d], %s\n", data, w ? "ax" : "al");
			} else {
				printf("mov %s, [%d]\n", w ? "ax" : "al", data);
			}

			continue;
		}
		// MOV segment registers to register/memory
		if ((c & 0b11111101) == OP_MOV_SEG_TO_RM) {
			d = (c >> 1) & 1;

			c = fgetc(input);
			if (c == EOF) {
				fprintf(stderr, "Unexpected EOF after opcode\n");
				fclose(input);
				return -1;
			}

			rm = c & 7;
			sr = (c >> 3) & 3;
			mod = (c >> 6);

			char *reg_name = segment_registers[sr];

			switch (mod) {
			case 0b11:	// register to register
				char *rm_name = registers[(w << 3) + rm];
				char *src = d ? rm_name : reg_name;
				char *dst = d ? reg_name : rm_name;
				printf("mov %s, %s\n", dst, src);
				break;

			case 0b01:	// memory mode, 8 bit displacement
				get_8bit_displacement(ea, rm, input);
				printf("mov %s, %s\n", d ? reg_name : ea, d ? ea : reg_name);
				break;

			case 0b10:	// memory mode, 16 bit displacement
				get_16bit_displacement(ea, rm, input);
				printf("mov %s, %s\n", d ? reg_name : ea, d ? ea : reg_name);
				break;

			case 0b00:	// memory mode, no displacement
				if (rm == 0b110) {
					int lo = fgetc(input);
					int hi = fgetc(input);
					if (lo == EOF || hi == EOF) {
						fprintf(stderr, "Unexpected EOF reading address\n");
						fclose(input);
						return -1;
					}
					uint16_t addr = lo | (hi << 8);
					sprintf(ea, "[%d]", addr);
				} else {
					sprintf(ea, "[%s]", ea_base[rm]);
				}
				printf("mov %s, %s\n", d ? reg_name : ea, d ? ea : reg_name);
				break;

			default:
				fprintf(stderr, "Unsupported addressing mode: %b\n", mod);
				return -1;
			}

			continue;
		}

		fprintf(stderr, "Unsupported instruction %b (%x)\n", c, c);
		return -1;
	}

}
