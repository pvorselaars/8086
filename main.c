#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define OP_MOV_RM_TO_RM   0b10001000
#define OP_MOV_IMMEDIATE  0b10110000
#define OP_MOV_IMM_TO_RM  0b11000110
#define OP_MOV_MEM_TO_ACC 0b10100000
#define OP_MOV_SEG_TO_RM  0b10001100

#define OP_ARITH_IMM_TO_RM 0b10000000

#define OP_ADD_RM_TO_RM   0b00000000
#define OP_ADD_IMM_TO_ACC 0b00000100

#define OP_SUB_RM_TO_RM	  0b00101000
#define OP_SUB_IMM_TO_ACC 0b00101100

#define OP_CMP_RM_TO_RM	  0b00111000
#define OP_CMP_IMM_TO_ACC 0b00111100

#define OP_COND_JUMP			0b01110000
#define OP_COND_LOOP			0b11100000

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

static char *imm_op[] = {
	"add",
	"or",
	"adc",
	"sbb",
	"and",
	"sub",
	"xor",
	"cmp"
};

static char *jumps[] = {
	"jo",
	"jno",
	"jb",
	"jnb",
	"je",
	"jne",
	"jbe",
	"ja",
	"js",
	"jns",
	"jp",
	"jnp",
	"jl",
	"jnl",
	"jle",
	"jg"
};

static char *loops[] = {
	"loopnz",
	"loopz",
	"loop",
	"jcxz",
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

uint16_t read_word(FILE *f)
{
	int lo = fgetc(f);
	int hi = fgetc(f);
	if (lo == EOF || hi == EOF) {
		fprintf(stderr, "Unexpected EOF\n");
		exit(1);
	}
	return lo | (hi << 8);
}

int16_t get_immediate(FILE *f, bool w, bool sign_extend)
{
	if (sign_extend && w) {
		int byte = fgetc(f);
		if (byte == EOF) {
			fprintf(stderr, "Unexpected EOF\n");
			exit(1);
		}
		return (int8_t) byte;
	}

	if (w) {
		return (int16_t) read_word(f);
	} else {
		int byte = fgetc(f);
		if (byte == EOF) {
			fprintf(stderr, "Unexpected EOF\n");
			exit(1);
		}
		return byte;
	}
}

bool decode_rm(FILE *f, uint8_t mod, uint8_t rm, bool w, char *ea_out, char **reg_out)
{
	if (mod == 0b11) {
		*reg_out = registers[(w << 3) + rm];
		return true;
	}

	int16_t disp = 0;

	if (mod == 0b00 && rm == 0b110) {
		uint16_t addr = read_word(f);
		sprintf(ea_out, "[%u]", addr);
	} else {
		if (mod == 0b01) {
			disp = (int8_t) fgetc(f);
		} else if (mod == 0b10) {
			disp = (int16_t) read_word(f);
		}

		if (disp == 0) {
			sprintf(ea_out, "[%s]", ea_base[rm]);
		} else if (disp > 0) {
			sprintf(ea_out, "[%s + %d]", ea_base[rm], disp);
		} else {
			sprintf(ea_out, "[%s - %d]", ea_base[rm], -disp);
		}
	}

	return false;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <file>\n", argv[0]);
		return 1;
	}

	FILE *input = fopen(argv[1], "r");

	if (!input) {
		perror("Error opening file");
		return -1;
	}

	printf("; %s\nbits 16\n", argv[1]);

	int32_t c;
	while ((c = fgetc(input)) != EOF) {
		uint8_t opcode = c;

		// MOV register/memory to/from register
		if ((opcode & 0b11111100) == OP_MOV_RM_TO_RM) {
			bool d = (opcode >> 1) & 1;
			bool w = opcode & 1;

			uint8_t modrm = fgetc(input);
			uint8_t mod = modrm >> 6;
			uint8_t reg = (modrm >> 3) & 7;
			uint8_t rm = modrm & 7;

			char ea[64];
			char *rm_operand, *reg_operand = registers[(w << 3) + reg];

			bool is_reg = decode_rm(input, mod, rm, w, ea, &rm_operand);

			char *src = d ? (is_reg ? rm_operand : ea) : reg_operand;
			char *dst = d ? reg_operand : (is_reg ? rm_operand : ea);

			printf("mov %s, %s\n", dst, src);
			continue;
		}
		// MOV immediate to register
		if ((opcode & 0b11110000) == OP_MOV_IMMEDIATE) {
			bool w = (opcode >> 3) & 1;
			uint8_t reg = opcode & 7;
			int16_t data = get_immediate(input, w, false);

			printf("mov %s, %d\n", registers[(w << 3) + reg], data);
			continue;
		}
		// MOV immediate to register/memory
		if ((opcode & 0b11111110) == OP_MOV_IMM_TO_RM) {
			bool w = opcode & 1;

			uint8_t modrm = fgetc(input);
			uint8_t mod = modrm >> 6;
			uint8_t rm = modrm & 7;

			char ea[64];
			char *rm_operand;
			bool is_reg = decode_rm(input, mod, rm, w, ea, &rm_operand);

			int16_t data = get_immediate(input, w, false);

			if (is_reg) {
				printf("mov %s, %d\n", rm_operand, data);
			} else {
				printf("mov %s %s, %d\n", w ? "word" : "byte", ea, data);
			}
			continue;
		}
		// MOV memory to/from accumulator
		if ((opcode & 0b11111100) == OP_MOV_MEM_TO_ACC) {
			bool d = (opcode >> 1) & 1;
			bool w = opcode & 1;
			uint16_t addr = read_word(input);

			char *acc = w ? "ax" : "al";
			if (d) {
				printf("mov [%u], %s\n", addr, acc);
			} else {
				printf("mov %s, [%u]\n", acc, addr);
			}
			continue;
		}
		// MOV segment register to/from register/memory
		if ((opcode & 0b11111101) == OP_MOV_SEG_TO_RM) {
			bool d = (opcode >> 1) & 1;

			uint8_t modrm = fgetc(input);
			uint8_t mod = modrm >> 6;
			uint8_t sr = (modrm >> 3) & 3;
			uint8_t rm = modrm & 7;

			char ea[64];
			char *rm_operand, *seg_reg = segment_registers[sr];

			bool is_reg = decode_rm(input, mod, rm, true, ea, &rm_operand);

			char *src = d ? (is_reg ? rm_operand : ea) : seg_reg;
			char *dst = d ? seg_reg : (is_reg ? rm_operand : ea);

			printf("mov %s, %s\n", dst, src);
			continue;
		}
		// ADD register/memory to/from register
		if ((opcode & 0b11111100) == OP_ADD_RM_TO_RM) {
			bool d = (opcode >> 1) & 1;
			bool w = opcode & 1;

			uint8_t modrm = fgetc(input);
			uint8_t mod = modrm >> 6;
			uint8_t reg = (modrm >> 3) & 7;
			uint8_t rm = modrm & 7;

			char ea[64];
			char *rm_operand, *reg_operand = registers[(w << 3) + reg];

			bool is_reg = decode_rm(input, mod, rm, w, ea, &rm_operand);

			char *src = d ? (is_reg ? rm_operand : ea) : reg_operand;
			char *dst = d ? reg_operand : (is_reg ? rm_operand : ea);

			printf("add %s, %s\n", dst, src);
			continue;
		}
		// ADD immediate to accumulator
		if ((opcode & 0b11111110) == OP_ADD_IMM_TO_ACC) {
			bool w = opcode & 1;
			bool s = false;

			int16_t data = get_immediate(input, w, s);

			printf("add %s %s, %d\n", w ? "word" : "byte", w ? "ax" : "al", data);
			continue;
		}
		// SUB register/memory to/from register
		if ((opcode & 0b11111100) == OP_SUB_RM_TO_RM) {
			bool d = (opcode >> 1) & 1;
			bool w = opcode & 1;

			uint8_t modrm = fgetc(input);
			uint8_t mod = modrm >> 6;
			uint8_t reg = (modrm >> 3) & 7;
			uint8_t rm = modrm & 7;

			char ea[64];
			char *rm_operand, *reg_operand = registers[(w << 3) + reg];

			bool is_reg = decode_rm(input, mod, rm, w, ea, &rm_operand);

			char *src = d ? (is_reg ? rm_operand : ea) : reg_operand;
			char *dst = d ? reg_operand : (is_reg ? rm_operand : ea);

			printf("sub %s, %s\n", dst, src);
			continue;
		}
		// SUB immediate to accumulator
		if ((opcode & 0b11111110) == OP_SUB_IMM_TO_ACC) {
			bool w = opcode & 1;
			bool s = false;

			int16_t data = get_immediate(input, w, s);

			printf("sub %s %s, %d\n", w ? "word" : "byte", w ? "ax" : "al", data);
			continue;
		}
		// CMP register/memory to/from register
		if ((opcode & 0b11111100) == OP_CMP_RM_TO_RM) {
			bool d = (opcode >> 1) & 1;
			bool w = opcode & 1;

			uint8_t modrm = fgetc(input);
			uint8_t mod = modrm >> 6;
			uint8_t reg = (modrm >> 3) & 7;
			uint8_t rm = modrm & 7;

			char ea[64];
			char *rm_operand, *reg_operand = registers[(w << 3) + reg];

			bool is_reg = decode_rm(input, mod, rm, w, ea, &rm_operand);

			char *src = d ? (is_reg ? rm_operand : ea) : reg_operand;
			char *dst = d ? reg_operand : (is_reg ? rm_operand : ea);

			printf("cmp %s, %s\n", dst, src);
			continue;
		}
		// CMP immediate to accumulator
		if ((opcode & 0b11111110) == OP_CMP_IMM_TO_ACC) {
			bool w = opcode & 1;
			bool s = false;

			int16_t data = get_immediate(input, w, s);

			printf("cmp %s %s, %d\n", w ? "word" : "byte", w ? "ax" : "al", data);
			continue;
		}
		// ARITHMETIC immediate to register/memory
		if ((opcode & 0b11111100) == OP_ARITH_IMM_TO_RM) {
			bool w = opcode & 1;
			bool s = (opcode >> 1) & 1;

			uint8_t modrm = fgetc(input);
			uint8_t mod = modrm >> 6;
			uint8_t opext = (modrm >> 3) & 7;
			uint8_t rm = modrm & 7;

			char *op = imm_op[opext];
			char ea[64];
			char *rm_operand;
			bool is_reg = decode_rm(input, mod, rm, w, ea, &rm_operand);

			int16_t data = get_immediate(input, w, s);

			if (is_reg) {
				printf("%s %s, %d\n", op, rm_operand, data);
			} else {
				printf("%s %s %s, %d\n", op, w ? "word" : "byte", ea, data);
			}
			continue;
		}

		if ((opcode & 0b11110000) == OP_COND_JUMP) {
			int8_t offset = get_immediate(input, false, true);
			printf("%s $%+d\n", jumps[opcode & 0b00001111], offset + 2);
			continue;
		}

		if ((opcode & 0b11110000) == OP_COND_LOOP) {
			int8_t offset = get_immediate(input, false, true);
			printf("%s $%+d\n", loops[opcode & 0b00000011], offset + 2);
			continue;
		}

		fprintf(stderr, "Unsupported instruction:  0x%02x\n", opcode);
		return 1;
	}

	fclose(input);
	return 0;
}
