// Compile With  
// g++ -o teste teste.cpp -lssl -lcrypto -lsecp256k1          

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <unistd.h>
#include <csignal>
#include <thread>
#include <random>
#include <secp256k1.h>
#include <iostream>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <chrono>
#include "base58.cpp"
#include "base58.h"

// Global Variables
std::string const hex_chars = "0123456789abcdef";


// char target_address[35] = "13DiRx5F1b1J8QWKATyLJZSAepKw1PkRbF"; //Test address
// std::string const partial_key = "xxxxxx084d812356c128ba06a4192587b75a984fd08dbe31af8e9d4e70810ab2"; // Teste Key


char target_address[34] = "1Hoyt6UBzwL5vvUSTLMQC2mwvvE5PpeSC";
std::string const partial_key = "403b3d4xcxfx6x9xfx3xaxcx5x0x4xbxbx7x2x6x8x7x8xax4x0x8x3x3x3x7x3x";


std::vector<unsigned char> decoded_target_address;
bool success = decodeBase58(target_address, decoded_target_address);

volatile int found = 0;
pthread_mutex_t file_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER; 

std::string last_key;

std::vector<int> icounter = {0};

// Threads Args
typedef struct
{
    int thread_id;
    int refresh_time;
    int batch_size;
} ThreadArgs;

// Input Verifications
int get_valid_input(const char *prompt, int default_value, int is_int)
{
    char input[10];
    int value;

    printf("%s", prompt);
    fgets(input, sizeof(input), stdin);

    if (sscanf(input, "%d", &value) == 1 && value > 0)
    {
        return value;
    }
    return default_value;
}

// Função para gerar as chaves
void generate_random_key(std::vector<std::string> &output_key) {
    static std::random_device rd;
    static std::mt19937 gen(rd()+140695); // Gerador de números aleatórios Mersenne Twister

    for(int position=0; position<output_key.size(); position++){
        // output_key[position].resize(64);

        for (int i = 0; i < 64; i++) {
            if (partial_key[i] == 'x') {
                output_key[position][i] = hex_chars[(gen() % 16)]; // Substitui 'x' por um caractere aleatório
            } else {
                output_key[position][i] = partial_key[i]; // Mantém os caracteres que não são 'x'
            }
        }
    }
}

// Função para calcular o SHA-256
std::vector<uint8_t> sha256(const std::vector<uint8_t> &data)
{
    std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data.data(), data.size());
    SHA256_Final(hash.data(), &ctx);
    return hash;
}

// Função para calcular o RIPEMD-160
std::vector<uint8_t> ripemd160(const std::vector<uint8_t> &data)
{
    std::vector<uint8_t> hash(RIPEMD160_DIGEST_LENGTH);
    RIPEMD160_CTX ctx;
    RIPEMD160_Init(&ctx);
    RIPEMD160_Update(&ctx, data.data(), data.size());
    RIPEMD160_Final(hash.data(), &ctx);
    return hash;
}


// Converter hex para bytes
std::vector<uint8_t> hexToBytes(const std::string &hex)
{
    std::vector<uint8_t> bytes(hex.length() / 2);
    for (size_t i = 0; i < bytes.size(); i++)
    {
        sscanf(&hex[i * 2], "%2hhx", &bytes[i]);
    }
    return bytes;
}



// Função principal para converter uma chave privada em endereço Bitcoin
void privateKeyToBitcoinAddress(std::vector<std::vector<uint8_t>> &generated_addresses, std::vector<std::string> &generated_keys)
{
    for (int i=0; i < generated_keys.size(); i++){
        std::vector<uint8_t> privateKeyBytes = hexToBytes(generated_keys[i]);

        secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);

        // Gerar a chave pública
        secp256k1_pubkey pubkey;
        if (!secp256k1_ec_pubkey_create(ctx, &pubkey, privateKeyBytes.data()))
        {
            throw std::runtime_error("Erro ao gerar chave pública.");
        }

        // Serializar a chave pública em formato comprimido
        std::vector<uint8_t> publicKey(33);
        size_t publicKeyLen = publicKey.size();
        secp256k1_ec_pubkey_serialize(ctx, publicKey.data(), &publicKeyLen, &pubkey, SECP256K1_EC_COMPRESSED);

        secp256k1_context_destroy(ctx);

        // Aplicar SHA-256 e RIPEMD-160 na chave pública
        std::vector<uint8_t> sha256Hash = sha256(publicKey);
        std::vector<uint8_t> ripemd160Hash = ripemd160(sha256Hash);

        // Adicionar o prefixo de rede (0x00 para Bitcoin mainnet)
        std::vector<uint8_t> prefixedHash = {0x00};
        prefixedHash.insert(prefixedHash.end(), ripemd160Hash.begin(), ripemd160Hash.end());

        // Calcular o checksum
        std::vector<uint8_t> checksumInput = sha256(sha256(prefixedHash));
        std::vector<uint8_t> checksum(checksumInput.begin(), checksumInput.begin() + 4);

        // Concatenar o hash prefixado com o checksum
        prefixedHash.insert(prefixedHash.end(), checksum.begin(), checksum.end());

        generated_addresses.push_back(prefixedHash);
    }
}

// Função de comparação entre o endereço gerado e o alvo
int check_key(std::vector<std::string> &generated_keys)
{

    std::vector<std::vector<uint8_t>> generated_addresses;
    privateKeyToBitcoinAddress(generated_addresses, generated_keys);
    // Converte a chave privada de hexadecimal para bytes

    
    for (int i=0; i < generated_keys.size(); i++) {
        
        if (generated_addresses[i] == decoded_target_address){
            return i;
        };

    }

    return 0;

}

// Worker
void *bruteforce_worker(void *args)
{
    ThreadArgs *thread_args = (ThreadArgs *)args;

    std::vector<std::string> generated_key(thread_args->batch_size, std::string(64, ' '));
    
    std::this_thread::sleep_for(std::chrono::milliseconds((thread_args->thread_id + 1) * 37));

    while (!found)
    { // Continue enquanto nenhuma thread encontrar a chave
        generate_random_key(generated_key);
        if (int position = check_key(generated_key))
        {
            found = 1; // Sinaliza que a chave foi encontrada
            
            // Protege a escrita no arquivo com o mutex
            pthread_mutex_lock(&file_lock);
            printf("\nThread %d encontrou a chave: %s\n", thread_args->thread_id, generated_key[position].c_str());

            FILE *file = fopen("key.txt", "w");
            if (file != NULL)
            {
                fprintf(file, "Chave privada encontrada: %s\n", generated_key[position].c_str());
                fclose(file);
            }
            pthread_mutex_unlock(&file_lock);
            kill(0, SIGKILL);
            break; // Sai do loop
        }

        icounter[thread_args->thread_id] += thread_args->batch_size;
        last_key = generated_key[0];

    }

    return nullptr;
}

// int test() {
//     auto start_time = std::chrono::high_resolution_clock::now();
//     int testcount = 0;
//     for (int i=0; i<500000; i++){
//         std::string generated_key(64,' ');
//         generate_random_key(generated_key);
//         check_key(generated_key);
//         testcount ++;
//     }
//     auto current_time = std::chrono::high_resolution_clock::now();

//     std::chrono::duration<double> elapsed = current_time - start_time;
//     double keys_per_second = testcount / elapsed.count();

//     std::cout << "Keys Generated per Second: " << keys_per_second << std::endl;
//     return 0;
// }

int main()
{
    try{

        // test();
        // return 1;

        int refresh_time = get_valid_input("Taxa de atualização (em segundos): ", 2, 1);
        int num_threads = get_valid_input("Quantidade de Threads (Padrão 6): ", 6, 1);
        int num_processes = get_valid_input("Quantidade de Processos (Padrão 2): ", 2, 1);
        int batch_size = 4096; //get_valid_input("Tamanho do lote: ", 4096, 1);
            
        icounter.resize(num_threads);

        pid_t pid;

        for (int i=1; i < num_processes; i++){
            pid = fork();
            if (pid == 0){
                break;
            }
        }

        // Configura as threads
        pthread_t threads[num_threads]; 
        ThreadArgs thread_args[num_threads];
        for (int i = 0; i < num_threads; i++)
        {
            thread_args[i].thread_id = i;
            thread_args[i].refresh_time = refresh_time;
            thread_args[i].batch_size = batch_size;

            pthread_create(&threads[i], nullptr, bruteforce_worker, &thread_args[i]);
        }

        auto start_time = std::chrono::high_resolution_clock::now();
        if ( pid != 0 || num_processes == 1){
            while (!found) {
                std::this_thread::sleep_for(std::chrono::milliseconds(refresh_time * 500));

                auto current_time = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed = current_time - start_time;
                int total;
                for (int i=0; i < icounter.size(); i++){
                    total += icounter[i];
                    icounter[i] = 0;
                }
                double keys_per_second = (total * num_processes) / elapsed.count();

                std::cout << "\r" << keys_per_second << " Keys/s" << " Last Key Checked: " << last_key << "         " << std::flush;
                total = 0;
                start_time = std::chrono::high_resolution_clock::now();
                std::this_thread::sleep_for(std::chrono::milliseconds(refresh_time * 500));
            }

        }

        // Aguarda todas as threads finalizarem
        for (int i = 0; i < num_threads; i++)
        {
            pthread_join(threads[i], nullptr);
        }

        pthread_mutex_destroy(&file_lock);

        std::cout << "\nProcesso concluído." << std::endl;
    }catch(const std::exception &e){
        std::cout << "\nErro: " << e.what() << std::endl;
    }
    catch(...){
        printf("\nErro não identificado\n");
    }

    return 0;
}
