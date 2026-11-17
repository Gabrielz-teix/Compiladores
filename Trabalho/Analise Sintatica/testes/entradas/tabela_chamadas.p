fn main() {
    let a, b: int;
    let media: float;
    let inicial: char;
    a = 4;
    b = dobro(a);
    imprimir(a, b, 'x');
    b = dobro(dobro(a));
    media = calcula_media(a, b, 2.5);
    inicial = 'c';
    println("{} {}", a, dobro(b));
    desconhecida(a);
}

fn dobro(n: int) -> int {
    return n * 2;
}

fn calcula_media(x: int, y: int, peso: float) -> float {
    let soma: float;
    soma = peso;
    return soma;
}

fn imprimir(p1: int, p2: int, p3: char) {
    println("{} {} {}", p1, p2, p3);
}
