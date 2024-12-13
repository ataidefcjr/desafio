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
#include <fstream>
#include "base58.h"

int batch_size = 65536;
int refresh_time = 2;
int num_threads = 4;
int num_processes = 3; 

// Global Variables
static secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
std::string const hex_chars = "0123456789abcdef";

std::string partial_key;
std::string target_address;
std::vector<unsigned char> decoded_target_address;

volatile int found = 0;
pthread_mutex_t file_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER; 

std::string last_key;

std::vector<int> global_counter = {0};


// Threads Args
typedef struct
{
    int thread_id;
    int refresh_time;
    int batch_size;
} ThreadArgs;

struct KeyConfig {
    std::string partial_key;
    std::string target_address;
};

KeyConfig readConfigFromFile(const std::string &filename) {
    std::ifstream file(filename);
    KeyConfig config;
    
    if (file.is_open()) {
        std::getline(file, config.partial_key);
        std::getline(file, config.target_address);
        file.close();
    } else {
        throw std::runtime_error("Não foi possível abrir o arquivo " + filename);
    }
    
    if (config.partial_key.empty() || config.target_address.empty()) {
        throw std::runtime_error("Arquivo de configuração incompleto");
    }
    
    return config;
}


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
void privateKeyToBitcoinAddress(std::vector<std::vector<uint8_t>> &generated_addresses, std::vector<std::string> &generated_keys, int thread_id)
{

    std::vector<uint8_t> publicKey(33);
    std::vector<uint8_t> sha256Buffer(32);
    std::vector<uint8_t> ripemd160Buffer(20);
    std::vector<uint8_t> prefixedHash(21);
    std::vector<uint8_t> finalHash(25);

    RIPEMD160_CTX rctx;
    SHA256_CTX sctx;


    for (int i = 0; i < generated_keys.size(); i++) {
        std::vector<uint8_t> privateKeyBytes = hexToBytes(generated_keys[i]);

        secp256k1_pubkey pubkey;
        if (!secp256k1_ec_pubkey_create(ctx, &pubkey, privateKeyBytes.data())) {
            throw std::runtime_error("Erro ao gerar chave pública.");
        }

        size_t publicKeyLen = publicKey.size();
        secp256k1_ec_pubkey_serialize(ctx, publicKey.data(), &publicKeyLen, &pubkey, SECP256K1_EC_COMPRESSED);

        // SHA256 da chave pública
        SHA256_Init(&sctx);
        SHA256_Update(&sctx, publicKey.data(), publicKey.size());
        SHA256_Final(sha256Buffer.data(), &sctx);
        
        // RIPEMD160
        RIPEMD160_Init(&rctx);
        RIPEMD160_Update(&rctx, sha256Buffer.data(), sha256Buffer.size());
        RIPEMD160_Final(ripemd160Buffer.data(), &rctx);

        // Adiciona prefixo de rede (0x00 para mainnet)
        prefixedHash[0] = 0x00;
        std::copy(ripemd160Buffer.begin(), ripemd160Buffer.end(), prefixedHash.begin() + 1);

        SHA256_Init(&sctx);
        SHA256_Update(&sctx, prefixedHash.data(), prefixedHash.size());
        SHA256_Final(sha256Buffer.data(), &sctx);
        
        SHA256_Init(&sctx);
        SHA256_Update(&sctx, sha256Buffer.data(), sha256Buffer.size());
        SHA256_Final(sha256Buffer.data(), &sctx);


        // Monta o endereço final (versão + hash + checksum)
        std::copy(prefixedHash.begin(), prefixedHash.end(), finalHash.begin());
        std::copy(sha256Buffer.begin(), sha256Buffer.begin() + 4, finalHash.begin() + 21);

        generated_addresses[i] = finalHash;

        global_counter[thread_id] ++;
    }
}

// Função de comparação entre o endereço gerado e o alvo
int check_key(std::vector<std::string> &generated_keys, int thread_id)
{

    std::vector<std::vector<uint8_t>> generated_addresses(batch_size);
    privateKeyToBitcoinAddress(generated_addresses, generated_keys, thread_id);
    // Converte a chave privada de hexadecimal para bytes

    
    for (int i=0; i < batch_size; i++) {
        
        if (generated_addresses[i] == decoded_target_address){
            return i;
        }
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
        if (int position = check_key(generated_key, thread_args->thread_id))
        {
            found = 1; // Sinaliza que a chave foi encontrada
            
            // Protege a escrita no arquivo com o mutex
            printf("\nThread %d encontrou a chave: %s\n", thread_args->thread_id, generated_key[position].c_str());

            pthread_mutex_lock(&file_lock);
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


        if (thread_args->thread_id == 0){
            last_key = generated_key[0];
        }

    }

    return nullptr;
}


int main()
{
    try{
        if (!num_threads) {
        num_threads = get_valid_input("Quantidade de Threads (Padrão 6): ", 6, 1);
        }
        if(!num_processes){
        num_processes = get_valid_input("Quantidade de Processos: ", 12, 1);
        }
            
        KeyConfig config = readConfigFromFile("partialkey.txt");
        partial_key = config.partial_key;
        target_address = config.target_address;

        bool success = decodeBase58(target_address, decoded_target_address);
        
        global_counter.resize(num_threads);

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


        if ( pid != 0 || num_processes == 1){

            std::cout << "Starting search on Address: " << target_address << std::endl;
            std::cout << "Partial Key: " << partial_key << std::endl;
            auto start_time = std::chrono::high_resolution_clock::now();

            while (!found) {
                std::this_thread::sleep_for(std::chrono::milliseconds(refresh_time * 1000));

                auto current_time = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed = current_time - start_time;
                int total;
                for (int i=0; i < global_counter.size(); i++){
                    pthread_mutex_lock(&counter_lock);
                    total += global_counter[i];
                    global_counter[i] = 0;
                    pthread_mutex_unlock(&counter_lock);
                }
                double keys_per_second = (total * num_processes) / elapsed.count();

                std::cout << "\rSpeed: " << keys_per_second << " Keys/s   " << std::flush;
                total = 0;
                start_time = std::chrono::high_resolution_clock::now();
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
