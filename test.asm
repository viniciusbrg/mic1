        bipush 21
        istore x
        bipush 12
rep     bipush 3
        iadd
        istore y
        iload y
        iload x
        if_icmpeq fim
        iload y
        goto rep
fim     nop