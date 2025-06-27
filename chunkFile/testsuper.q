class father{
    method(){
        print "aaaaaaa";
    }

}
class son < father{
    nmethed(){
        super.method();
    }
    
}

son().nmethed();