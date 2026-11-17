fn main() {
    let inteiro, outro_valor2: int;
    let real: float;
    let letra: char;
    inteiro = 0;
    outro_valor2 = 123;
    real = 3.14;
    letra = 'a';
    letra = '+';
    letra = '_';
    letra = ' ';
    if inteiro == outro_valor2 { inteiro = inteiro + 1; }
    else if inteiro != 1 { inteiro = inteiro - 1; }
    else { inteiro = inteiro * 2 / 1; }
    while inteiro < 10 { inteiro = inteiro + 1; }
    while inteiro <= 10 { inteiro = inteiro + 1; }
    if inteiro > 0 { real = real; }
    if inteiro >= 0 { real = (real); }
    println("valor = {} e {}", inteiro, letra);
}

fn identidade(a: int) -> int {
    return a;
}
