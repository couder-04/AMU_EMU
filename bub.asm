; Bubble sort five values in ascending order.
        ldc 0x1000
        a2sp
        adj -6
        ldc 5
        stl 2
        ldc 0
        stl 0

outer:  ldl 2
        adc -1
        stl 5
        ldl 0
        ldl 5
        sub
        brlz outerbody
        br done

outerbody:
        ldc 0
        stl 1
inner:  ldl 2
        ldl 0
        sub
        adc -1
        stl 5
        ldl 1
        ldl 5
        sub
        brlz innerbody
        br outernext

innerbody:
        ldc values
        ldl 1
        add
        ldnl 0
        stl 3
        ldc values
        ldl 1
        add
        adc 1
        ldnl 0
        stl 5
        ldl 3
        ldl 5
        sub
        brlz noswap
        brz noswap

        ldc values
        ldl 1
        add
        stl 4
        ldl 5
        ldl 4
        stnl 0
        ldc values
        ldl 1
        add
        adc 1
        stl 4
        ldl 3
        ldl 4
        stnl 0

noswap:
        ldl 1
        adc 1
        stl 1
        br inner

outernext:
        ldl 0
        adc 1
        stl 0
        br outer

done:   halt

values: data 5
        data 1
        data 4
        data 2
        data 3