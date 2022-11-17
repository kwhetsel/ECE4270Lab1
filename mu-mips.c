#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "mu-mips.h"

/***************************************************************/
/* Print out a list of commands available                                                                  */
/***************************************************************/
void help() {
	printf("------------------------------------------------------------------\n\n");
	printf("\t**********MU-MIPS Help MENU**********\n\n");
	printf("sim\t-- simulate program to completion \n");
	printf("run <n>\t-- simulate program for <n> instructions\n");
	printf("rdump\t-- dump register values\n");
	printf("reset\t-- clears all registers/memory and re-loads the program\n");
	printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
	printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
	printf("high <val>\t-- set the HI register to <val>\n");
	printf("low <val>\t-- set the LO register to <val>\n");
	printf("print\t-- print the program loaded into memory\n");
	printf("show\t-- print the current content of the pipeline registers\n");
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory                                                                            */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) &&  ( address <= MEM_REGIONS[i].end) ) {
			uint32_t offset = address - MEM_REGIONS[i].begin;
			return (MEM_REGIONS[i].mem[offset+3] << 24) |
					(MEM_REGIONS[i].mem[offset+2] << 16) |
					(MEM_REGIONS[i].mem[offset+1] <<  8) |
					(MEM_REGIONS[i].mem[offset+0] <<  0);
		}
	}
	return 0;
}

/***************************************************************/
/* Write a 32-bit word to memory                                                                                */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
	int i;
	uint32_t offset;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) && (address <= MEM_REGIONS[i].end) ) {
			offset = address - MEM_REGIONS[i].begin;

			MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
			MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
			MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
			MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
		}
	}
}

/***************************************************************/
/* Execute one cycle                                                                                                              */
/***************************************************************/
void cycle() {
	handle_pipeline();
	CURRENT_STATE = NEXT_STATE;
	CYCLE_COUNT++;
}

/***************************************************************/
/* Simulate MIPS for n cycles                                                                                       */
/***************************************************************/
void run(int num_cycles) {

	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped\n\n");
		return;
	}

	printf("Running simulator for %d cycles...\n\n", num_cycles);
	int i;
	for (i = 0; i < num_cycles; i++) {
		if (RUN_FLAG == FALSE) {
			printf("Simulation Stopped.\n\n");
			break;
		}
		cycle();
	}
}

/***************************************************************/
/* simulate to completion                                                                                               */
/***************************************************************/
void runAll() {
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped.\n\n");
		return;
	}

	printf("Simulation Started...\n\n");
	while (RUN_FLAG){
		cycle();
	}
	printf("Simulation Finished.\n\n");
}

/***************************************************************/
/* Dump a word-aligned region of memory to the terminal                              */
/***************************************************************/
void mdump(uint32_t start, uint32_t stop) {
	uint32_t address;

	printf("-------------------------------------------------------------\n");
	printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
	printf("-------------------------------------------------------------\n");
	printf("\t[Address in Hex (Dec) ]\t[Value]\n");
	for (address = start; address <= stop; address += 4){
		printf("\t0x%08x (%d) :\t0x%08x\n", address, address, mem_read_32(address));
	}
	printf("\n");
}

/***************************************************************/
/* Dump current values of registers to the teminal                                              */
/***************************************************************/
void rdump() {
	int i;
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
	printf("# Cycles Executed\t: %u\n", CYCLE_COUNT);
	printf("PC\t: 0x%08x\n", CURRENT_STATE.PC);
	printf("-------------------------------------\n");
	printf("[Register]\t[Value]\n");
	printf("-------------------------------------\n");
	for (i = 0; i < MIPS_REGS; i++){
		printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
	}
	printf("-------------------------------------\n");
	printf("[HI]\t: 0x%08x\n", CURRENT_STATE.HI);
	printf("[LO]\t: 0x%08x\n", CURRENT_STATE.LO);
	printf("-------------------------------------\n");
}

/***************************************************************/
/* Read a command from standard input.                                                               */
/***************************************************************/
void handle_command() {
	char buffer[20];
	uint32_t start, stop, cycles;
	uint32_t register_no;
	int register_value;
	int hi_reg_value, lo_reg_value;

	printf("MU-MIPS SIM:> ");

	if (scanf("%s", buffer) == EOF){
		exit(0);
	}

	switch(buffer[0]) {
		case 'S':
		case 's':
			if (buffer[1] == 'h' || buffer[1] == 'H'){
				show_pipeline();
			}else {
				runAll();
			}
			break;
		case 'M':
		case 'm':
			if (scanf("%x %x", &start, &stop) != 2){
				break;
			}
			mdump(start, stop);
			break;
		case '?':
			help();
			break;
		case 'Q':
		case 'q':
			printf("**************************\n");
			printf("Exiting MU-MIPS! Good Bye...\n");
			printf("**************************\n");
			exit(0);
		case 'R':
		case 'r':
			if (buffer[1] == 'd' || buffer[1] == 'D'){
				rdump();
			}else if(buffer[1] == 'e' || buffer[1] == 'E'){
				reset();
			}
			else {
				if (scanf("%d", &cycles) != 1) {
					break;
				}
				run(cycles);
			}
			break;
		case 'I':
		case 'i':
			if (scanf("%u %i", &register_no, &register_value) != 2){
				break;
			}
			CURRENT_STATE.REGS[register_no] = register_value;
			NEXT_STATE.REGS[register_no] = register_value;
			break;
		case 'H':
		case 'h':
			if (scanf("%i", &hi_reg_value) != 1){
				break;
			}
			CURRENT_STATE.HI = hi_reg_value;
			NEXT_STATE.HI = hi_reg_value;
			break;
		case 'L':
		case 'l':
			if (scanf("%i", &lo_reg_value) != 1){
				break;
			}
			CURRENT_STATE.LO = lo_reg_value;
			NEXT_STATE.LO = lo_reg_value;
			break;
		case 'P':
		case 'p':
			print_program();
			break;
		default:
			printf("Invalid Command.\n");
			break;
	}
}

/***************************************************************/
/* reset registers/memory and reload program                                                    */
/***************************************************************/
void reset() {
	int i;
	/*reset registers*/
	for (i = 0; i < MIPS_REGS; i++){
		CURRENT_STATE.REGS[i] = 0;
	}
	CURRENT_STATE.HI = 0;
	CURRENT_STATE.LO = 0;

	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}

	/*load program*/
	load_program();

	/*reset PC*/
	INSTRUCTION_COUNT = 0;
	CURRENT_STATE.PC =  MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/***************************************************************/
/* Allocate and set memory to zero                                                                            */
/***************************************************************/
void init_memory() {
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		MEM_REGIONS[i].mem = malloc(region_size);
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
}

/**************************************************************/
/* load program into memory                                                                                      */
/**************************************************************/
void load_program() {
	FILE * fp;
	int i, word;
	uint32_t address;

	/* Open program file. */
	fp = fopen(prog_file, "r");
	if (fp == NULL) {
		printf("Error: Can't open program file %s\n", prog_file);
		exit(-1);
	}

	/* Read in the program. */

	i = 0;
	while( fscanf(fp, "%x\n", &word) != EOF ) {
		address = MEM_TEXT_BEGIN + i;
		mem_write_32(address, word);
		printf("writing 0x%08x into address 0x%08x (%d)\n", word, address, address);
		i += 4;
	}
	PROGRAM_SIZE = i/4;
	printf("Program loaded into memory.\n%d words written into memory.\n\n", PROGRAM_SIZE);
	fclose(fp);
}

/************************************************************/
/* maintain the pipeline                                                                                           */
/************************************************************/
void handle_pipeline()
{
	/*INSTRUCTION_COUNT should be incremented when instruction is done*/
	/*Since we do not have branch/jump instructions, INSTRUCTION_COUNT should be incremented in WB stage */

	WB();
	MEM();
	EX();
	ID();
	IF();
}

/************************************************************/
/* writeback (WB) pipeline stage:                                                                          */
/************************************************************/
void WB()
{
	/*IMPLEMENT THIS*/
	uint32_t opcode = MEM_WB.IR << 26;
	uint32_t rt = (MEM_WB.IR << 11) >> 27;
	if(opcode == 0xf || opcode == 0x20 || opcode == 0x21 || opcode == 0x23){ //load
		NEXT_STATE.REGS[rt] = MEM_WB.LMD;
	}
	else if(opcode == 0){
		uint32_t rd = (MEM_WB.IR << 16) >> 27;
		NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
	}
	else{
		NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
	}
	INSTRUCTION_COUNT++;
}
/************************************************************/
/* memory access (MEM) pipeline stage:                                                          */
/************************************************************/
void MEM()
{
	uint32_t opcode = EX_MEM.IR << 26;
	MEM_WB.IR = EX_MEM.IR;
	if(opcode == 0xf || opcode == 0x20 || opcode == 0x21 || opcode == 0x23){ //load
		MEM_WB.LMD = mem_read_32(EX_MEM.ALUOutput);
	}
	else if(opcode == 0x28 || opcode == 0x29 || opcode == 0x2b){ //store
		mem_write_32(EX_MEM.ALUOutput, NEXT_STATE.REGS[EX_MEM.B]);
	}
	else{ // ALU ops
		MEM_WB.ALUOutput = EX_MEM.ALUOutput;
	}
	/*IMPLEMENT THIS*/
}

/************************************************************/
/* execution (EX) pipeline stage:                                                                          */
/************************************************************/
void EX()
{
	uint32_t opcode = ID_EX.IR >> 26;
	EX_MEM.IR = ID_EX.IR;
	if(opcode == 0){
		uint32_t funct = (ID_EX.IR << 26) >> 26;
		uint32_t shamt = (ID_EX.IR << 21) >> 27;
		uint32_t rd = (ID_EX.IR << 16) >> 27;
		switch(funct){
				case 0x20: //add
				case 0x21: //addu
					EX_MEM.ALUOutput = NEXT_STATE.REGS[ID_EX.A] + NEXT_STATE.REGS[NEXT_STATE.REGS[ID_EX.B]];
					break;

				case 0x22: //sub
				case 0x23: //subu
					EX_MEM.ALUOutput = NEXT_STATE.REGS[ID_EX.A] - NEXT_STATE.REGS[ID_EX.B];
					break;

				case 0x24: //and
					EX_MEM.ALUOutput = NEXT_STATE.REGS[ID_EX.A] & NEXT_STATE.REGS[ID_EX.B];
					break;

				case 0x25: //or
					EX_MEM.ALUOutput = NEXT_STATE.REGS[ID_EX.A] | NEXT_STATE.REGS[ID_EX.B];
					break;

				case 0x26: //xor
					EX_MEM.ALUOutput = NEXT_STATE.REGS[ID_EX.A] ^ NEXT_STATE.REGS[ID_EX.B];
					break;

				case 0x27: //nor
					EX_MEM.ALUOutput =~ NEXT_STATE.REGS[ID_EX.A] | NEXT_STATE.REGS[ID_EX.B];
					break;

				case 0x2a: //slt
					if(rd != 0) //um idk
						EX_MEM.ALUOutput = ((int)NEXT_STATE.REGS[ID_EX.A] < (int)NEXT_STATE.REGS[ID_EX.B]) ? 1 : 0;

					break;

				case 0x2b: //sltu
					if(rd != 0) //um idk
						EX_MEM.ALUOutput = (NEXT_STATE.REGS[ID_EX.A] < NEXT_STATE.REGS[ID_EX.B]) ? 1 : 0;
					break;

				case 0x00: //sll
					if(rd != 0)
						EX_MEM.ALUOutput = NEXT_STATE.REGS[ID_EX.B] << shamt;
					NEXT_STATE.PC +=4;
					break;

				case 0x02: //srl
					if(rd != 0)
						EX_MEM.ALUOutput = NEXT_STATE.REGS[ID_EX.B] >> shamt;
					break;

				case 0x03: //srl
					if(rd != 0)
						EX_MEM.ALUOutput = (int)NEXT_STATE.REGS[ID_EX.B] >> shamt;
					break;

				case 0x18: //mult
					EX_MEM.ALUOutput = NEXT_STATE.REGS[ID_EX.A] * NEXT_STATE.REGS[ID_EX.B];
            		break;

				case 0x19: //multu
					EX_MEM.ALUOutput = NEXT_STATE.REGS[ID_EX.A] * NEXT_STATE.REGS[ID_EX.B];
            		break;

				case 0x1a: //div
					print_instruction(CURRENT_STATE.PC);
					if (NEXT_STATE.REGS[ID_EX.B] != 0)
					{
						EX_MEM.ALUOutput = NEXT_STATE.REGS[ID_EX.A] / NEXT_STATE.REGS[ID_EX.B];
					}
					else
						printf("Division by zero. Value unpredictable");
					break;

				case 0x1b: //divu
					if (NEXT_STATE.REGS[ID_EX.B] != 0)
					{
						EX_MEM.ALUOutput = NEXT_STATE.REGS[ID_EX.A] / NEXT_STATE.REGS[ID_EX.B];
						/*NEXT_STATE.HI = NEXT_STATE.REGS[rs] % NEXT_STATE.REGS[rt];
						NEXT_STATE.LO = NEXT_STATE.REGS[rs] / NEXT_STATE.REGS[rt];*/
					}
					else
						printf("Division by zero. Value unpredictable");
					break;
				case 0x0c: //syscall
					if (NEXT_STATE.REGS[2] == 10)
						RUN_FLAG = FALSE;
					break;

			}
	}
	else if(opcode == 0x8 || opcode == 0x9){	//Immeidate ALU
		EX_MEM.ALUOutput = NEXT_STATE.REGS[ID_EX.A] + ID_EX.imm; //addi and addiu
	}
	else if(opcode == 0xc){
		EX_MEM.ALUOutput = NEXT_STATE.REGS[ID_EX.A] & ID_EX.imm; //ani
	}
	else if(opcode == 0xd){
		EX_MEM.ALUOutput = NEXT_STATE.REGS[ID_EX.A] | ID_EX.imm; // ori
	}
	else if(opcode == 0xe){
		EX_MEM.ALUOutput = NEXT_STATE.REGS[ID_EX.A] ^ ID_EX.imm; //xori
	}
	else{	//load/store
		EX_MEM.ALUOutput = NEXT_STATE.REGS[ID_EX.A] + ID_EX.imm;
		EX_MEM.B = NEXT_STATE.REGS[ID_EX.B];
	}


	/*IMPLEMENT THIS*/
}

/************************************************************/
/* instruction decode (ID) pipeline stage:                                                         */
/************************************************************/
void ID()
{
	uint32_t opcode = IF_ID.IR >> 26;
	ID_EX.IR = IF_ID.IR;
	ID_EX.A = (IF_ID.IR << 6) >> 27;
	ID_EX.B = (IF_ID.IR << 11) >> 27;


	if(opcode != 0){
		ID_EX.imm = (IF_ID.IR << 16) >> 16;
	}
}

/************************************************************/
/* instruction fetch (IF) pipeline stage:                                                              */
/************************************************************/
void IF()
{
	NEXT_STATE = CURRENT_STATE;
	IF_ID.IR = mem_read_32(NEXT_STATE.PC);
	NEXT_STATE.PC += 4;
}


/************************************************************/
/* Initialize Memory                                                                                                    */
/************************************************************/
void initialize() {
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in MIPS assembly format)    */
/************************************************************/
void print_program(){
	int i;
	uint32_t addr;

	for(i=0; i<PROGRAM_SIZE; i++){
		addr = MEM_TEXT_BEGIN + (i*4);
		printf("[0x%x]\t", addr);
		print_instruction(addr);
	}
}

/************************************************************/
/* Print the current pipeline                                                                                    */
/************************************************************/
void show_pipeline(){
	/*IMPLEMENT THIS*/
	printf("Current PC:\t %x\n", CURRENT_STATE.PC);
	printf("IF/ID.IR:\t %s\n", IF_ID.IR);
	printf("IF/ID.PC:\t %u\n\n", IF_ID.PC);

	printf("ID/EX.IR:\t %u\n", ID_EX.IR);
	printf("ID/EX.A:\t %u\n", ID_EX.A);
	printf("ID/EX.B:\t %u\n", ID_EX.B);
	printf("ID/EX.imm:\t %u\n\n", ID_EX.imm);

	printf("EX/MEM.IR:\t %u\n", EX_MEM.IR);
	printf("EX/MEM.A:\t %u\n", EX_MEM.A);
	printf("EX/MEM.B:\t %u\n", EX_MEM.B);
	printf("EX/MEM.ALUOutput:\t %u\n\n", EX_MEM.ALUOutput);

	printf("MEM/WB.IR:\t %u\n", MEM_WB.IR);
	printf("MEM/WB.ALUOutput:\t %u\n", MEM_WB.ALUOutput);
	printf("MEM/WB.LMD:\t %u\n", MEM_WB.LMD);
	
}


/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {
	printf("\n**************************\n");
	printf("Welcome to MU-MIPS SIM...\n");
	printf("**************************\n\n");

	if (argc < 2) {
		printf("Error: You should provide input file.\nUsage: %s <input program> \n\n",  argv[0]);
		exit(1);
	}

	strcpy(prog_file, argv[1]);
	initialize();
	load_program();
	help();
	while (1){
		handle_command();
	}
	return 0;
}
