sll $0, $0, 0 # NOP
addi $1, $0 -1
addi $2, $0, 0x7FFF
add  $3, $1, $2
sub  $4, $1, $2
and  $4, $1, $2
or   $5, $3, $2
xor  $4, $4, $3
nor  $4, $4, $4
nand $4, $4, $4
slt  $5, $1, $4
sll  $2, $2, 3
srl  $1, $1, 2
sra  $1, $1, 1
andi $4, $4, 0x7FFF
lui  $1, 0xFFFF
ori  $1, $1, 0xFFFF
nori  $4, $4, 0xFFFF
slti $5, $4, -0x8000
addiu $s0, $0, 0xFFFF # modified mult open-testcase
addiu $s1, $0, 0xFFFF
mult $s0, $s1
mfhi $0
mflo $s2
multu $s0, $s1
addi $s0, $0, 0xFFFE
addi $s1, $0, -2
mult $s0, $s1
multu $s0, $s1
mflo $0
mflo $s2
addi $sp, $sp, -4 # modified recur open-testcase
sw $ra, 0($sp)
lw $a0, 0($0)
jal sumToN
sw $v0, 4($0)
lw $ra, 0($sp)
addi $sp, $sp, 4
lw $v1, 4($0)
j finish
halt
halt
halt
halt
halt
sumToN: slti $t0, $a0, 2
        beq  $t0, $0, recur
        or   $v0, $a0, $0
        jr   $ra
 recur: addi $sp, $sp, -8
        sw   $ra, 4($sp)
        sw   $a0, 0($sp)
        addi $a0, $a0, -1
        jal  sumToN
        lw   $a0, 0($sp)
        lw   $ra, 4($sp)
        addi $sp, $sp, 8
        add  $v0, $v0, $a0
        jr   $ra
finish: lw $t1, 1($0)
lb $t1, 2($0)
lbu $t1, 3($0)
sw $t1, 4($0)
halt
halt
halt
halt
halt
halt
halt
