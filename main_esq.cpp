#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>

using namespace std;

//definiçes de tipos
typedef unsigned char byte;
typedef unsigned int  word;
typedef unsigned long microcode;

//estrutura para guardar uma microinstruçao decodificada
struct decoded_microcode
{
    word nadd; // addr 
    byte jam; // jam
    byte sft; // shift ?
    byte alu; // uka
    word reg_w; // "c"
    byte mem; // mem
    byte reg_r; // "b"
};

//Funções utilitárias ======================
void write_microcode(microcode w) //Dado uma microinstrucao, exibe na tela devidamente espaçado pelas suas partes.
{
   unsigned int v[36];
   for(int i = 35; i >= 0; i--)
   {
       v[i] = (w & 1);
       w = w >> 1;
   }

   for(int i = 0; i < 36; i++)
   {
       cout << v[i];
       if(i == 8 || i == 11 || i == 13 || i == 19 || i == 28 || i == 31) cout << " ";
   }
}

void write_word(word w) //Dada uma palavra (valor de 32 bits / 4 bytes), exibe o valor binário correspondente.
{
   unsigned int v[32];
   for(int i = 31; i >= 0; i--)
   {
       v[i] = (w & 1);
       w = w >> 1;
   }

   for(int i = 0; i < 32; i++)
       cout << v[i];
}

void write_byte(byte b) //Dado um byte (valor de 8 bits), exibe o valor binário correspondente na tela.
{
   unsigned int v[8];
   for(int i = 7; i >= 0; i--)
   {
       v[i] = (b & 1);
       b = b >> 1;
   }

   for(int i = 0; i < 8; i++)
       cout << v[i];
}

void write_dec(word d) //Dada uma palavra (valor de 32 bits / 4 bytes), exibe o valor decimal correspondente.
{
   cout << (int)d << endl;
}
//=========================================

//sinalizador para desligar máquina
bool halt = false;

//memoria principal
#define MEM_SIZE 0xFFFF+1 //0xFFFF + 0x1; // 64 KBytes = 64 x 1024 Bytes = 65536 (0xFFFF+1) x 1 Byte;
byte memory[MEM_SIZE]; //0x0000 a 0xFFFF (0 a 65535)

//registradores
word mar=0, mdr=0, pc=0, sp=0, lv=0, cpp=0, tos=0, opc=0, h=0;
byte mbr=0;

//barramentos
word bus_a=0, bus_b=0, bus_c=0, alu_out=0;

//estado da ALU para salto condicional
byte n=0, z=1;

//registradores de microprograma
word mpc;

//memória de microprograma: 512 x 64 bits = 512 x 8 bytes = 4096 bytes = 4 KBytes.
//Cada microinstrução é armazenada em 8 bytes (64 bits), mas apenas os 4,5 bytes (36 bits) de ordem mais baixa são de fato decodificados.
//Os 28 bits restantes em cada posição da memória são ignorados, mas podem ser utilizados para futuras melhorias nas microinstruções para controlar microarquiteturas mais complexas.
microcode microprog[512];

//carrega microprograma
//Escreve um microprograma de controle na memória de controle (array microprog, declarado logo acima)
void load_microprog() {
    //implementar!
    FILE *microprog_file  = fopen("microprog.rom", "rb");

    if (microprog_file == NULL) {
        printf("Problema na leitura do arquivo. Abortando...");
        exit(EXIT_FAILURE);
    }

    int microprog_size = 512;

    // cada microinstrução está representada como 64 bytes
    // onde os 36 bits menos significantes estão 
    // setados e os demais são ignorados.
    unsigned long instrucao_atual;
    unsigned long bitmask_instrucao = (1L << 36) - 1;
    for (int i = 0; i < microprog_size; i++) {
        fread(&instrucao_atual, sizeof(unsigned long), 1, microprog_file);
        microprog[i] = instrucao_atual & bitmask_instrucao; 
    }
}

void load_microprog_aulas() {
        // aula 9
    microprog[0] = 0b000000000100001101010000001000010001;
    microprog[1] = 0b000000010000001101010000001000010001;
    microprog[2] = 0b000000011000000101001000000000000010;
    microprog[3] = 0b000000100000001101010000001000010001;
    microprog[4] = 0b000000101000000101000100000000000010;
    microprog[5] = 0b000000110000001111000000000100000010;
    microprog[6] = 0b000000111000000101000100000000000001;
    microprog[7] = 0b000001000000001101100100000000001000;
    microprog[8] = 0b000001001000001101100100000000001000;
    microprog[9] = 0b000001010000000101000000000010001000;
    microprog[10] = 0b00001011000001100010000000001001000;

    memory[1] = 1;
    memory[2] = 0b11;
    memory[3] = 0b10;

    // aula 10
    //MAIN
    // microprog[0] =  0b000000000100001101010000001000010001; //PC <- PC + 1; fetch; GOTO MBR;

    // //OPC = OPC + memory[end_word];
    // microprog[2] =  0b000000011000001101010000001000010001; //PC <- PC + 1; fetch;
    // microprog[3] =  0b000000100000000101000000000010100010; //MAR <- MBR; read;
    // microprog[4] =  0b000000101000000101001000000000000000; //H <- MDR;
    // microprog[5] =  0b000000000000001111000100000000001000; //OPC <- OPC + H; GOTO MAIN;

    // //memory[end_word] = OPC;
    // microprog[6] =  0b000000111000001101010000001000010001; //PC <- PC + 1; fetch;
    // microprog[7] =  0b000001000000000101000000000010000010; //MAR <- MBR;
    // microprog[8] =  0b000000000000000101000000000101001000; //MDR <- OPC; write; GOTO MAIN;

    // //goto endereco_comando_programa;
    // microprog[9] =  0b000001010000001101010000001000010001; //PC <- PC + 1; fetch;
    // microprog[10] = 0b000000000100000101000000001000010010; //PC <- MBR; fetch; GOTO MBR;

    // //if OPC = 0 goto endereco_comando_programa else goto proxima_linha;
    // microprog[11] = 0b000001100001000101000100000000001000; //OPC <- OPC; IF ALU = 0 GOTO 268 (100001100) 								                ELSE GOTO 12 (000001100);
    // microprog[12] = 0b000000000000001101010000001000000001; //PC <- PC + 1; GOTO MAIN;
    // microprog[268] = 0b100001101000001101010000001000010001; //PC <- PC + 1; fetch;
    // microprog[269] = 0b000000000100000101000000001000010010; //PC <- MBR; fetch; GOTO MBR;

    // //OPC = OPC - memory[end_word];
    // microprog[13] = 0b000001110000001101010000001000010001; //PC <- PC + 1; fetch;
    // microprog[14] = 0b000001111000000101000000000010100010; //MAR <- MBR; read;
    // microprog[15] = 0b000010000000000101001000000000000000; //H <- MDR;
    // microprog[16] = 0b000000000000001111110100000000001000; //OPC <- OPC - H; GOTO MAIN;

    // memory[1] = 0x02;
    // memory[2] = 0x0A;
    // memory[3] = 0x02;
    // memory[4] = 0x0B;
    // memory[5] = 0x06;
    // memory[6] = 0x0C;
    // memory[40] = 0x05;
    // memory[44] = 0x03;

    // memoria aula 14
    //     memory[0] = 0x00; 
    // memory[1] = 0x73; 
    // memory[2] = 0x00; 
    // memory[3] = 0x00;
    // memory[4] = 0x06; 
    // memory[5] = 0x00; 
    // memory[6] = 0x00; 
    // memory[7] = 0x00; 
    // memory[8] = 0x01; 
    // memory[9] = 0x10; 
    // memory[10] = 0x00; 
    // memory[11] = 0x00; 
    // memory[12] = 0x00; 
    // memory[13] = 0x04; 
    // memory[14] = 0x00; 
    // memory[15] = 0x00; 
    // memory[16] = 0x03; 
    // memory[17] = 0x10; 
    // memory[18] = 0x00; 
    // memory[19] = 0x00;
    // memory[1025] = 0x19;
    // memory[1026] = 0x15;
    // memory[1027] = 0x22;
    // memory[1028] = 0x00;
    // memory[1029] = 0x19;
    // memory[1030] = 0x0C;
    // memory[1031] = 0x19;
    // memory[1032] = 0x03;
    // memory[1033] = 0x02;
    // memory[1034] = 0x22;
    // memory[1035] = 0x01;
    // memory[1036] = 0x1C;
    // memory[1037] = 0x01;
    // memory[1038] = 0x1C;
    // memory[1039] = 0x00;
    // memory[1040] = 0x4B;
    // memory[1041] = 0x00;
    // memory[1042] = 0x08;
    // memory[1043] = 0x1C;
    // memory[1044] = 0x01;
    // memory[1045] = 0x3C;
    // memory[1046] = 0xFF;
    // memory[1047] = 0xF2;
    // memory[1048] = 0x01;
}

//carrega programa na memória principal para ser executado pelo emulador.
//programa escrito em linguagem de máquina (binário) direto na memória principal (array memory declarado mais acima).
void load_prog() {
    //implementar!
    FILE *prog_file  = fopen("test.bin", "rb");

    if (prog_file == NULL) {
        printf("Problema na leitura do arquivo. Abortando...");
        exit(EXIT_FAILURE);
    }

    // pegar o valor de Q, primeiros 4 bytes do arquivo.
    unsigned int q;
    fread(&q, sizeof(int), 1, prog_file);

    // primeiros 20 bytes do arquivo após header são os bytes de 
    // inicialização
    byte byte_atual;
    for (int i = 0; i < 20; i++) {
        fread(&byte_atual, sizeof(byte), 1, prog_file);
        memory[i] = byte_atual & 0xFF;
    }

    int remaining_bytes = q - 20;
    for (int j = 0; j < remaining_bytes; j++) {
        fread(&byte_atual, sizeof(byte), 1, prog_file);
        memory[1025 + j] = byte_atual;
    }
}

//exibe estado da máquina
void debug(bool clr = true) {
    if(clr) system("clear");

    cout << "Microinstrução: ";
    write_microcode(microprog[mpc]);

    cout << "\n\nMemória principal: \nPilha: \n";
    for(int i = lv*4; i <= sp*4; i+=4)
    {
        write_byte(memory[i+3]);
        cout << " ";
        write_byte(memory[i+2]);
        cout << " ";
        write_byte(memory[i+1]);
        cout << " ";
        write_byte(memory[i]);
        cout << " : ";
        if(i < 10) cout << " ";
        cout << i << " | " << memory[i+3] << " " << memory[i+2] << " " << memory[i+1] << " " << memory[i];
        word w;
        memcpy(&w, &memory[i], 4);
        cout << " | " << i/4 << " : " << w << endl;
    }

    cout << "\n\nPC: \n";
    for(int i = (pc-1); i <= pc+20; i+=4)
    {
        write_byte(memory[i+3]);
        cout << " ";
        write_byte(memory[i+2]);
        cout << " ";
        write_byte(memory[i+1]);
        cout << " ";
        write_byte(memory[i]);
        cout << " : ";
        if(i < 10) cout << " ";
        cout << i << " | " << memory[i+3] << " " << memory[i+2] << " " << memory[i+1] << " " << memory[i];
        word w;
        memcpy(&w, &memory[i], 4);
        cout << " | " << i/4 << " : " << w << endl;
    }

    cout << "\nRegistradores - \nMAR: " << mar << " ("; write_word(mar);
    cout << ") \nMDR: " << mdr << " ("; write_word(mdr);
    cout << ") \nPC : " << pc << " ("; write_word(pc);
    cout << ") \nMBR: " << (int) mbr << " ("; write_byte(mbr);
    cout << ") \nSP : " << sp << " (";  write_word(sp);
    cout << ") \nLV : " << lv << " ("; write_word(lv);
    cout << ") \nCPP: " << cpp << " ("; write_word(cpp);
    cout << ") \nTOS: " << tos << " ("; write_word(tos);
    cout << ") \nOPC: " << opc << " ("; write_word(opc);
    cout << ") \nH  : " << h << " ("; write_word(h);
    cout << ")" << endl;
}

decoded_microcode decode_microcode(microcode code) { //Recebe uma microinstrução binária e separa suas partes preenchendo uma estrutura de microinstrucao decodificada, retornando-a.
    // parsear o microcódigo (36 bits) sempre pegando os bits menos significativos, usando shifts e mascaras de bits pra filtrar os bits desejados.
    decoded_microcode dec;

    microcode current_code = code;

    dec.reg_r = current_code & 0b1111;
    current_code = current_code >> 4;
    dec.mem = current_code & 0b111;
    current_code = current_code >> 3;
    dec.reg_w = current_code & 0b111111111;
    current_code = current_code >> 9;
    dec.alu = current_code & 0b111111;
    current_code = current_code >> 6;
    dec.sft = current_code & 0b11;
    current_code = current_code >> 2;
    dec.jam = current_code & 0b111;
    current_code = current_code >> 3;
    dec.nadd = current_code & 0b111111111;

    return dec;
}

//alu
//recebe uma operação de alu binária representada em um byte (ignora-se os 2 bits de mais alta ordem, pois a operação é representada em 6 bits)
//e duas palavras (as duas entradas da alu), carregando no barramento alu_out o resultado da respectiva operação aplicada às duas palavras.
void alu(byte func, word a, word b)
{
    // implementando as funções conforme tabela da ula do livro
    /**
     * Bits da esquerda pra direita
     * F0, F1, ENA, ENB, INVA, INC
     * 
    **/
    byte funcao = func & 0b111111;
    switch (funcao) {
        case 0b011000:
            // A
            alu_out = a;
            break;
        case 0b010100:
            // B
            alu_out = b;
            break;
        case 0b011010:
            // Not(a)
            alu_out = ~a;
            break;
        case 0b101100:
            // not b
            alu_out = ~b;
            break;
        case 0b111100:
            // a + b
            alu_out = a + b;
            break;
        case 0b111101:
            // a + b + 1
            alu_out = a + b + 1;
            break;
        case 0b111001:
            // a + 1
            alu_out = a + 1;
            break;
        case 0b110101:
            // b + 1
            alu_out = b + 1;
            break;
        case 0b111111:
            // b - a
            alu_out = b - a;
            break;
        case 0b110110:
            // b - 1
            alu_out = b - 1;
            break;
        case 0b111011:
            // -a
            alu_out = -a;
            break;
        case 0b001100:
            // a and b
            alu_out = a & b;
            break;
        case 0b011100:
            // a or b
            alu_out = a | b;
            break;
        case 0b010000:
            // 0
            alu_out = 0;
            break;
        case 0b110001:
            // 1
            alu_out = 1;
            break;
        case 0b110010:
            // -1
            alu_out = -1;
            break;
    }

    // atualizar bits de estado: n e z
    if (alu_out == 0) {
        n = 0;
        z = 1;
    } else {
        n = 1;
        z = 0;
    }
}

//Deslocamento. Recebe a instrução binária de deslocamento representada em um byte (ignora-se os 6 bits de mais alta ordem, pois o deslocador eh controlado por 2 bits apenas)
//e uma palavra (a entrada do deslocador) e coloca o resultado no barramento bus_c.
void shift(byte s, word w)
{
    // S tem dois bits: SLL8 e SRA1, nessa ordem.
    // SLL8 = shift de 8 bits pra esquerda (1 byte pra esquerda)
    // SRA1 = shift de 1 bit pra direita
    // PS: BITS ESTÃO INVERTIDOS, ENTAO SLL8 = 0b01 e SRA1 = 0b10
    word result = w;
    if (s & 0b1) {
        result = result << 8;
    } 

    if (s & 0b10 != 0) {
        result = result >> 1;
    }

    bus_c = result;
}

//Leitura de registradores. Recebe o número do registrador a ser lido (0 = mdr, 1 = pc, 2 = mbr, 3 = mbru, ..., 8 = opc) representado em um byte,
//carregando o barramento bus_b (entrada b da ALU) com o valor do respectivo registrador e o barramento bus_a (entrada A da ALU) com o valor do registrador h.
void read_registers(byte reg_end)
{
    byte reg_selecionado = reg_end & 0b1111;

    bus_a = h;
    switch (reg_selecionado) {
        case 0:
            bus_b = mdr;
            break;
        case 1:
            bus_b = pc;
            break;
        case 2:
            // mbr sem sinal
            bus_b = mbr & 0xFF;
            break;
        case 3: {
            // mbr com sinal
            cout << "PASSOU AQ !";
            byte msb = mbr >> 7;

            if (msb == 0) {
                // 0 nos bits mais significativos, 
                // logo podemos por o valor da mbr direto no barramento
                bus_b = mbr & 0xFF;

            } else {
                // 1 é o bit mais signicativo
                // fazemos um binário com os 24 bits mais signicativos em 1 e 
                // as 8 posições finais preenchemos com nossos 8 bits originais
                // fazendo um ou bit a bit.
                bus_b = 0xFFFFFF00 | mbr;
            }
            break;
        }
        case 4:
            bus_b = sp;
            break;
        case 5:
            bus_b = lv;
            break;
        case 6:
            bus_b = cpp;
            break;
        case 7:
            bus_b = tos;
            break;
        case 8: 
            bus_b = opc;
            break; 
    }

}

//Escrita de registradores. Recebe uma palavra (valor de 32 bits) cujos 9 bits de ordem mais baixa indicam quais dos 9 registradores que
//podem ser escritos receberao o valor que está no barramento bus_c (saída do deslocador).
void write_register(word reg_end)
{
    word regs_possiveis = reg_end & 0b111111111;

    // vamos escolher registrador por registrador quais serão escritos;
    byte write_mar = regs_possiveis & 0b1;
    regs_possiveis = regs_possiveis >> 1;
    byte write_mdr = regs_possiveis & 0b1;
    regs_possiveis = regs_possiveis >> 1;
    byte write_pc = regs_possiveis & 0b1;
    regs_possiveis = regs_possiveis >> 1;
    byte write_sp = regs_possiveis & 0b1;
    regs_possiveis = regs_possiveis >> 1;
    byte write_lv = regs_possiveis & 0b1;
    regs_possiveis = regs_possiveis >> 1;
    byte write_cpp = regs_possiveis & 0b1;
    regs_possiveis = regs_possiveis >> 1;
    byte write_tos = regs_possiveis & 0b1;
    regs_possiveis = regs_possiveis >> 1;
    byte write_opc = regs_possiveis & 0b1;
    regs_possiveis = regs_possiveis >> 1;
    byte write_h = regs_possiveis & 0b1;

    if (write_mar) {
        mar = bus_c;
    }

    if (write_mdr) {
        mdr = bus_c;
    }

    if (write_pc) {
        pc = bus_c;
    }

    if (write_sp) {
        sp = bus_c;
    }

    if (write_lv) {
        lv = bus_c;
    }

    if (write_cpp) {
        cpp = bus_c;
    }

    if (write_tos) {
        tos = bus_c;
    }

    if (write_opc) {
        opc = bus_c;
    }

    if (write_h) {
        h = bus_c;
    }
}

//Leitura e escrita de memória. Recebe em um byte o comando de memória codificado nos 3 bits de mais baixa ordem (fetch, read e write, podendo executar qualquer conjunto dessas três operações ao
//mesmo tempo, sempre nessa ordem) e executa a respectiva operação na memória principal.
//fetch: lê um byte da memória principal no endereço constando em PC para o registrador MBR. Endereçamento por byte.
//write e read: escreve e lê uma PALAVRA na memória principal (ou seja, 4 bytes em sequência) no endereço constando em MAR com valor no registrador MDR. Nesse caso, o endereçamento é dado em palavras.
//Mas, como a memoria principal eh um array de bytes, deve-se fazer a conversão do endereçamento de palavra para byte (por exemplo, a palavra com endereço 4 corresponde aos bytes 16, 17, 18 e 19).
//Lembrando que esta é uma máquina "little endian", isto é, os bits menos significativos são os de posições mais baixas.
//No exemplo dado, suponha os bytes:
//16 = 00110011
//17 = 11100011
//18 = 10101010
//19 = 01010101
//Tais bytes correspondem à palavra 01010101101010101110001100110011
void mainmemory_io(byte control)
{
    byte fetch = control & 0b1;
    byte read = (control & 0b10) >> 1;
    byte write = (control & 0b100) >> 2;

    if (fetch) {
        byte fetch_result = memory[pc];
        mbr = fetch_result;
    }

    if (read) {
        word initial_address = mar << 2;

        byte first_byte = memory[initial_address];
        byte second_byte = memory[initial_address + 1];
        byte third_byte = memory[initial_address + 2];
        byte final_byte = memory[initial_address + 3];

        word data = (final_byte << 24) | (third_byte << 16) | (second_byte << 8) | first_byte;
        mdr = data;
    }

    if (write) {
        word initial_address = mar << 2;

        // escrever os bits menos significantes nos menores endereços,
        // e ir empurrando os bits fazendo shifts, pegando de 8 em 8.
        word data_to_write = mdr;

        memory[initial_address] = data_to_write & 0xFF;
        data_to_write = data_to_write >> 8;
        memory[initial_address + 1] = data_to_write & 0xFF;
        data_to_write = data_to_write >> 8;
        memory[initial_address + 2] = data_to_write & 0xFF;
        data_to_write = data_to_write >> 8;
        memory[initial_address + 3] = data_to_write & 0xFF;
        data_to_write = data_to_write >> 8;
    }
}

//Define próxima microinstrução a ser executada. Recebe o endereço da próxima instrução a ser executada codificado em uma palavra (considera-se, portanto, apenas os 9 bits menos significativos)
//e um modificador (regra de salto) codificado em um byte (considera-se, portanto, apenas os 3 bits menos significativos, ou seja JAMZ (bit 0), JAMN (bit 1) e JMPC (bit 2)), construindo e
//retornando o endereço definitivo (codificado em uma word - desconsidera-se os 21 bits mais significativos (são zerados)) da próxima microinstrução.
word next_address(word next, byte jam)
{
    //implementar!
    if (jam == 0) {
        return next & 0b111111111;
    }

    byte coded_jam = jam;
    
    byte jamz = coded_jam & 0b1;
    coded_jam = coded_jam >> 1;
    byte jamn = coded_jam & 0b1;
    coded_jam = coded_jam >> 1;
    byte jmpc = coded_jam & 0b1;
    
    byte current_msb = (next & 0b111111111) >> 8;

    // deixar isso como word pra evitar possiveis erros de cast
    word new_msb = (jamz & z) | (jamn & n) | current_msb;
    word updated_address = (new_msb << 8) | next;

    if (jmpc) {
        updated_address = updated_address | mbr;
    }

    return updated_address;
}

int main(int argc, char* argv[])
{
    load_microprog(); //carrega microprograma de controle

    load_prog(); //carrega programa na memória principal a ser executado pelo emulador. Já que não temos entrada e saída, jogamos o programa direto na memória ao executar o emulador.

    decoded_microcode decmcode;

    //laço principal de execução do emulador. Cada passo no laço corresponde a um pulso do clock.
    //o debug mostra o estado interno do processador, exibindo valores dos registradores e da memória principal.
    //o getchar serve para controlar a execução de cada pulso pelo clique de uma tecla no teclado, para podermos visualizar a execução passo a passo.
    //Substitua os comentários pelos devidos comandos (na ordem dos comentários) para ter a implementação do laço.
    while(!halt)
    {
        if (pc == 1045) {
            printf("aq");
        }
        debug();

        // decode_microcode(0b1101100010110111001010010001000011100000010000111011100000001001);
        decmcode = decode_microcode(microprog[mpc]);//implementar! Pega uma microinstrução no armazenamento de controle no endereço determinado pelo registrador contador de microinstrução e decodifica (gera a estrutura de microinstrução, ou seja, separa suas partes). Cada parte é devidamente passada como parâmetro para as funções que vêm a seguir.
         //implementar! Lê registradores
        read_registers(decmcode.reg_r);
        //implementar! Executa alu
        alu(decmcode.alu, bus_a, bus_b);
        //implementar! Executa deslocamento
        shift(decmcode.sft, alu_out);
        //implementar! Escreve registradores
        write_register(decmcode.reg_w);
        //implementar! Manipula memória principal
        mainmemory_io(decmcode.mem);
        //implementar! Determina endereço da microinstrução (mpc) a ser executada no próximo pulso de clock
        mpc = next_address(decmcode.nadd, decmcode.jam);
	    getchar();
    }

    debug(false);

    return 0;
}
