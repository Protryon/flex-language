
pub module mod1 {
    pub class class1 {
        
    }

    priv class class2 {
        
    }

    prot class class3 {
        
    }

    typed synch virt iface class class4 {
        
    }
}
//comment
priv module mod2 {

    pub func uint32 namedFunc1(uint32 arg) {
        if (arg * arg > 25) arg * arg else arg / arg
    }

    pub func uint32 namedFunc2(uint32 arg = 5) {
        if (arg *== arg < 5) 5 else -5
    }

    pub func uint32 namedFunc3(uint32 arg = 5, uint32... more) uint32 ctr, (for (uint32 m : more) if (arg > m) ctr++), ctr
}

prot module mod3 {
    pub class class1 {

        priv class1[] internal_arr = [7];
        priv class1[] internal_arr = [8];
        priv uint32[] internal_arr2(7);
        synch uint32 synch_int = 5;
        csig uint32 csig_int = 456;
        uint32 cast_test = (uint32)(int32)5;

        pub func class1() {
            
        }

        pub <uint32 arg>class1 => {
            
        }

        pub func uint32 namedFunc1(uint32 arg) switch(arg) {
            case 1:
            break
            case 2:
            break
            case 3:
            arg * arg, break
        }

        pub func uint32 namedFunc2(uint32 arg = 5) for (uint32 i = 0; i < arg; i++) println(i)

        pub func uint32 namedFunc3(uint32 arg = 5, uint32... more) for (uint32 i = 0; i < arg && i < more.length; i++) println(more[i])
    }
}

module mod4.test {
    import mod3

    func class1 f() []
}

module mod4.test {

    func class1 f2() []
}