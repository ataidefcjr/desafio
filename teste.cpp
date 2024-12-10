// Compile With  
// g++ -o teste teste.cpp -lssl -lcrypto -lsecp256k1          

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <pthread.h>
#include <unistd.h>
#include <thread>
#include <time.h>
#include <random>
#include <string>
#include <cstdlib>
#include <secp256k1.h>
#include <iostream>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <chrono>
#include "base58.cpp"
#include "base58.h"

// Global Variables
const char *hex_chars = "0123456789abcdef";


// char target_address[35] = "13DiRx5F1b1J8QWKATyLJZSAepKw1PkRbF"; //Test address
// std::string partial_key = "3x91xb084d812356x128xa06a4192587b7xa984fd08dbx31af8e9d4e70810ab2"; // Teste Key


char target_address[34] = "1Hoyt6UBzwL5vvUSTLMQC2mwvvE5PpeSC";
std::string partial_key = "403b3x4xcxfx6x9xfx3xaxcx5x0x4xbxbx7x2x6x8x7x8xax4x0x8x3x3x3x7x3x";

std::vector<unsigned char> decoded_target_address;
bool success = decodeBase58(target_address, decoded_target_address);

volatile int found = 0;
pthread_mutex_t file_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t counter_lock; 

double global_counter = 0;
std::string last_key;

// Threads Args
typedef struct
{
    int thread_id;
    int refresh_time;
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
void generate_random_key(std::string &output_key) {
    static std::random_device rd; // Dispositivo para gerar números aleatórios
    static std::mt19937 gen(rd()); // Gerador de números aleatórios Mersenne Twister
    static std::uniform_int_distribution<> dis(0, 15); // Distribuição uniforme para números de 0 a 15

    int len = partial_key.length();

    output_key.resize(len); // Redimensiona o vetor de saída para o tamanho necessário
    for (int i = 0; i < len; i++) {
        if (partial_key[i] == 'x') {
            output_key[i] = hex_chars[dis(gen)]; // Substitui 'x' por um caractere aleatório
        } else {
            output_key[i] = partial_key[i]; // Mantém os caracteres que não são 'x'
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
std::vector<uint8_t> privateKeyToBitcoinAddress(const std::vector<uint8_t> &privateKey)
{
    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);

    // Gerar a chave pública
    secp256k1_pubkey pubkey;
    if (!secp256k1_ec_pubkey_create(ctx, &pubkey, privateKey.data()))
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

    return prefixedHash;
}

// Função de comparação entre o endereço gerado e o alvo
int check_key(std::string private_key)
{
    try
    {
        // Converte a chave privada de hexadecimal para bytes
        std::vector<uint8_t> privateKeyBytes = hexToBytes(private_key);

        // Verifica se a chave privada tem 32 bytes
        if (privateKeyBytes.size() != 32)
        {
            std::cerr << "A chave privada não tem 32 bytes!" << std::endl;
            return 0;
        }

        // Gera o endereço Bitcoin a partir da chave privada
        std::vector<uint8_t> generated_address = privateKeyToBitcoinAddress(privateKeyBytes);

        return (generated_address == decoded_target_address);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Erro ao verificar chave: " << e.what() << std::endl;
        return 0;
    }
}

// Worker
void *bruteforce_worker(void *args)
{
    ThreadArgs *thread_args = (ThreadArgs *)args;
    std::string generated_key; // Para armazenar a chave gerada
    int counter = 0;
    
    std::this_thread::sleep_for(std::chrono::milliseconds((thread_args->thread_id + 1) * 34));

    while (!found)
    { // Continue enquanto nenhuma thread encontrar a chave
        generate_random_key(generated_key);
        if (check_key(generated_key))
        {
            found = 1; // Sinaliza que a chave foi encontrada

            // Protege a escrita no arquivo com o mutex
            pthread_mutex_lock(&file_lock);
            printf("\nThread %d encontrou a chave: %s\n", thread_args->thread_id, generated_key.c_str());

            FILE *file = fopen("key.txt", "w");
            if (file != NULL)
            {
                fprintf(file, "Chave privada encontrada: %s\n", generated_key.c_str());
                fclose(file);
            }
            pthread_mutex_unlock(&file_lock);

            break; // Sai do loop
        }
        counter ++;

        if (counter > 1024 + ((thread_args->thread_id +1 ) * 37)) {
            pthread_mutex_lock(&counter_lock);
            global_counter += counter;
            counter = 0;
            pthread_mutex_unlock(&counter_lock);
            
            if (thread_args->thread_id == 0){
                    last_key = generated_key;
        }
        }

    }

    return nullptr;
}

int main()
{
    try{

        int refresh_time = get_valid_input("Taxa de atualização (em segundos): ", 1, 1);
        int num_threads = get_valid_input("Quantidade de Threads (Padrão 2): ", 2, 1);
        int num_processes = get_valid_input("Quantidade de Processos (Padrão 12): ", 12, 1);

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

            pthread_create(&threads[i], nullptr, bruteforce_worker, &thread_args[i]);
        }

        auto start_time = std::chrono::high_resolution_clock::now();
        
        while (!found) {
            
            auto current_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = current_time - start_time;

            double keys_per_second = global_counter / elapsed.count();

            std::cout << keys_per_second << " Keys/s" << " Last Key Checked: " << last_key << "        " << std::endl;
            global_counter = 0;
            start_time = std::chrono::high_resolution_clock::now();
            std::this_thread::sleep_for(std::chrono::seconds(refresh_time));
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
