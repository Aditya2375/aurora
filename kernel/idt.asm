; Interrupt stubs: save state, call C handler, restore, iret.
[BITS 32]
[SECTION .text]

extern isr_handler
extern irq_handler

%macro ISR_NOERR 1
global isr%1
isr%1:
    push dword 0
    push dword %1
    jmp isr_common
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    push dword %1
    jmp isr_common
%endmacro

%macro IRQ 2
global irq%1
irq%1:
    push dword 0
    push dword %2
    jmp irq_common
%endmacro

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_ERR   30
ISR_NOERR 31

IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

isr_common:
    pusha
    mov ax, ds
    push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    push esp
    call isr_handler
    add esp, 4
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 8
    iret

irq_common:
    pusha
    mov ax, ds
    push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    push esp
    call irq_handler
    add esp, 4
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 8
    iret

; Catch-all stubs for vectors 48-255: every IDT entry lands somewhere that
; reports the real vector number instead of triple-faulting on a zero gate.
global isr_default_table
%assign v 48
%rep 208
global isr_gen_ %+ v
isr_gen_ %+ v:
    push dword 0
    push dword v
    jmp isr_common
%assign v v+1
%endrep

; (table filled below, one entry per stub)

section .data
align 4
isr_default_table:
    dd isr_gen_48
    dd isr_gen_49
    dd isr_gen_50
    dd isr_gen_51
    dd isr_gen_52
    dd isr_gen_53
    dd isr_gen_54
    dd isr_gen_55
    dd isr_gen_56
    dd isr_gen_57
    dd isr_gen_58
    dd isr_gen_59
    dd isr_gen_60
    dd isr_gen_61
    dd isr_gen_62
    dd isr_gen_63
    dd isr_gen_64
    dd isr_gen_65
    dd isr_gen_66
    dd isr_gen_67
    dd isr_gen_68
    dd isr_gen_69
    dd isr_gen_70
    dd isr_gen_71
    dd isr_gen_72
    dd isr_gen_73
    dd isr_gen_74
    dd isr_gen_75
    dd isr_gen_76
    dd isr_gen_77
    dd isr_gen_78
    dd isr_gen_79
    dd isr_gen_80
    dd isr_gen_81
    dd isr_gen_82
    dd isr_gen_83
    dd isr_gen_84
    dd isr_gen_85
    dd isr_gen_86
    dd isr_gen_87
    dd isr_gen_88
    dd isr_gen_89
    dd isr_gen_90
    dd isr_gen_91
    dd isr_gen_92
    dd isr_gen_93
    dd isr_gen_94
    dd isr_gen_95
    dd isr_gen_96
    dd isr_gen_97
    dd isr_gen_98
    dd isr_gen_99
    dd isr_gen_100
    dd isr_gen_101
    dd isr_gen_102
    dd isr_gen_103
    dd isr_gen_104
    dd isr_gen_105
    dd isr_gen_106
    dd isr_gen_107
    dd isr_gen_108
    dd isr_gen_109
    dd isr_gen_110
    dd isr_gen_111
    dd isr_gen_112
    dd isr_gen_113
    dd isr_gen_114
    dd isr_gen_115
    dd isr_gen_116
    dd isr_gen_117
    dd isr_gen_118
    dd isr_gen_119
    dd isr_gen_120
    dd isr_gen_121
    dd isr_gen_122
    dd isr_gen_123
    dd isr_gen_124
    dd isr_gen_125
    dd isr_gen_126
    dd isr_gen_127
    dd isr_gen_128
    dd isr_gen_129
    dd isr_gen_130
    dd isr_gen_131
    dd isr_gen_132
    dd isr_gen_133
    dd isr_gen_134
    dd isr_gen_135
    dd isr_gen_136
    dd isr_gen_137
    dd isr_gen_138
    dd isr_gen_139
    dd isr_gen_140
    dd isr_gen_141
    dd isr_gen_142
    dd isr_gen_143
    dd isr_gen_144
    dd isr_gen_145
    dd isr_gen_146
    dd isr_gen_147
    dd isr_gen_148
    dd isr_gen_149
    dd isr_gen_150
    dd isr_gen_151
    dd isr_gen_152
    dd isr_gen_153
    dd isr_gen_154
    dd isr_gen_155
    dd isr_gen_156
    dd isr_gen_157
    dd isr_gen_158
    dd isr_gen_159
    dd isr_gen_160
    dd isr_gen_161
    dd isr_gen_162
    dd isr_gen_163
    dd isr_gen_164
    dd isr_gen_165
    dd isr_gen_166
    dd isr_gen_167
    dd isr_gen_168
    dd isr_gen_169
    dd isr_gen_170
    dd isr_gen_171
    dd isr_gen_172
    dd isr_gen_173
    dd isr_gen_174
    dd isr_gen_175
    dd isr_gen_176
    dd isr_gen_177
    dd isr_gen_178
    dd isr_gen_179
    dd isr_gen_180
    dd isr_gen_181
    dd isr_gen_182
    dd isr_gen_183
    dd isr_gen_184
    dd isr_gen_185
    dd isr_gen_186
    dd isr_gen_187
    dd isr_gen_188
    dd isr_gen_189
    dd isr_gen_190
    dd isr_gen_191
    dd isr_gen_192
    dd isr_gen_193
    dd isr_gen_194
    dd isr_gen_195
    dd isr_gen_196
    dd isr_gen_197
    dd isr_gen_198
    dd isr_gen_199
    dd isr_gen_200
    dd isr_gen_201
    dd isr_gen_202
    dd isr_gen_203
    dd isr_gen_204
    dd isr_gen_205
    dd isr_gen_206
    dd isr_gen_207
    dd isr_gen_208
    dd isr_gen_209
    dd isr_gen_210
    dd isr_gen_211
    dd isr_gen_212
    dd isr_gen_213
    dd isr_gen_214
    dd isr_gen_215
    dd isr_gen_216
    dd isr_gen_217
    dd isr_gen_218
    dd isr_gen_219
    dd isr_gen_220
    dd isr_gen_221
    dd isr_gen_222
    dd isr_gen_223
    dd isr_gen_224
    dd isr_gen_225
    dd isr_gen_226
    dd isr_gen_227
    dd isr_gen_228
    dd isr_gen_229
    dd isr_gen_230
    dd isr_gen_231
    dd isr_gen_232
    dd isr_gen_233
    dd isr_gen_234
    dd isr_gen_235
    dd isr_gen_236
    dd isr_gen_237
    dd isr_gen_238
    dd isr_gen_239
    dd isr_gen_240
    dd isr_gen_241
    dd isr_gen_242
    dd isr_gen_243
    dd isr_gen_244
    dd isr_gen_245
    dd isr_gen_246
    dd isr_gen_247
    dd isr_gen_248
    dd isr_gen_249
    dd isr_gen_250
    dd isr_gen_251
    dd isr_gen_252
    dd isr_gen_253
    dd isr_gen_254
    dd isr_gen_255
