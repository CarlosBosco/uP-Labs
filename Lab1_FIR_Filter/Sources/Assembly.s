;TODO: Floating point registers, how to import from fn, 
;and also change temp R7

	AREA FIR_asm, CODE, READONLY
	EXPORT __FIR_A
__FIR_A
	MOV R4, #0x0; top = 0
	ADD R5, R2, #0x14 ; 1+ Coeff max
	
__whileloop
	CMP R4, R3 ; compare top to length
	BEQ __return ; if conditions to exit loop met, return

__forloop
	CMP R2, R5 ;compare for loop reqs
	BEQ __incPtrs ;jumps back to while loop after for loop is done
	VLDR.32 S0, [R0, #0x0] ;load from input array into S0
	VLDR.32 S1, [R2, #0x0] ;load from coefficient array into S1
	VMUL.F32 S1, S0, S1 ;putting into temp curr coeff multiplied by curr input
	VLDR.32 S0, [R1, #0x0] ;load into S0 the contents of output array element
	VADD.F32 S1, S1, S0 ; running sum added into output
	VSTR.32 S1, [R1, #0x0]
	ADD R2, R2, #0x4 ; coefficient moving to next coefficient address
	SUB R0, R0, #0x4 ; Moving to prev InputArray element
	B __forloop
	
__incPtrs
	ADD R4, R4, #0x1 ; Add to top
	ADD R1, R1, #0x4 ; Move to next output array element
	ADD R0, R0, #0x18 ; increment pointer to start of next inputArray element
	SUB R2, R2, #0x14 ; Reset coeffArray position
	B __whileloop
	
__return

	END