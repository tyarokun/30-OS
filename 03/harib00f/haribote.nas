        ORG     0xc200

        MOV     SI,msg

putloop:
        MOV     AL,[SI]
        ADD     SI,1
        CMP     AL,0
        JE      fin
        MOV     AH,0x0e
        MOV     BX,15
        INT     0x10
        JMP     putloop

fin:
        HLT
        JMP     fin

msg:
        DB      0x0a, 0x0a
        DB      "haribote.sys loaded"
        DB      0x0a
        DB      0