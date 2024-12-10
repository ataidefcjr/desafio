#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <iostream>
#include <bitcoin/bitcoin.hpp>

// Global Variable
const char *target_address = "1Hoyt6UBzwL5vvUSTLMQC2mwvvE5PpeSC";
const char *hex_chars = "0123456789abcdef";
volatile int found = 0;
pthread_mutex_t file_lock = PTHREAD_MUTEX_INITIALIZER;

// Threads Args
typedef struct {
    char partial_key[65];
    int thread_id;
    int refresh_time;
} ThreadArgs;

// Input Verifications
int get_valid_input(const char *prompt, int default_value, int is_int) {
    char input[10];
    int value;

    printf("%s", prompt);
    fgets(input, sizeof(input), stdin);

    if (sscanf(input, "%d", &value) == 1 && value > 0) {
        return value;
    }
    return default_value;
}

// Generate Keys
void generate_random_key(char *partial_key, char *output_key, const char *hex_chars) {
    strcpy(output_key, partial_key); // Copia a chave parcial
    int len = strlen(partial_key);
    
    for (int i = 0; i < len; i++) {
        if (partial_key[i] == 'x') {
            output_key[i] = hex_chars[rand() % 16]; // Substitui 'x' por um caractere aleatório
        }
    }
}

// Função para converter uma string hexadecimal para bytes
int hex_to_bytes(const char *hex, unsigned char *bytes) {
    size_t len = strlen(hex);
    if (len % 2 != 0) return 0; // Hexadecimal precisa ter número par de caracteres

    for (size_t i = 0; i < len / 2; i++) {
        sscanf(&hex[i * 2], "%2hhx", &bytes[i]);
    }
    return 1;
}

// Função para verificar a chave
int check_key(const std::string& private_key_hex, const std::string& target_address) {
    // Cria a chave privada a partir da chave hexadecimal
    bc::ec_secret secret;
    if (!bc::decode_base16(secret, private_key_hex)) {
        std::cerr << "Erro ao decodificar chave privada." << std::endl;
        return 0;
    }

    // Gera a chave pública
    bc::wallet::ec_public public_key(secret);

    // Gera o endereço Bitcoin (padrão para mainnet)
    bc::wallet::payment_address address(public_key, bc::wallet::payment_address::mainnet_p2kh);

    // Converte o endereço para uma string
    std::string generated_address = address.encoded();

    // Compara o endereço gerado com o alvo
    return (generated_address == target_address);
}

// Worker
void *bruteforce_worker(void *args) {
    ThreadArgs *thread_args = (ThreadArgs *)args;
    char generated_key[65]; // Para armazenar a chave gerada

    while (!found) { // Continue enquanto nenhuma thread encontrar a chave
        generate_random_key(thread_args->partial_key, generated_key, hex_chars);

        if (check_key(generated_key, target_address)) {
            found = 1; // Sinaliza que a chave foi encontrada

            // Protege a escrita no arquivo com o mutex
            pthread_mutex_lock(&file_lock);
            printf("Thread %d encontrou a chave: %s\n", thread_args->thread_id, generated_key);

            FILE *file = fopen("key.txt", "w");
            if (file != NULL) {
                fprintf(file, "Chave privada encontrada: %s\n", generated_key);
                fclose(file);
            }
            pthread_mutex_unlock(&file_lock);

            break; // Sai do loop
        }
    }

    return NULL;
}

int main() {
    char partial_key[65] = "403b3x4xcxfx6x9xfx3xaxcx5x0x4xbxbx7x2x6x8x7x8xax4x0x8x3x3x3x7x3x";
    int refresh_time = get_valid_input("Taxa de atualização (em segundos): ", 1, 1);
    int num_processes = get_valid_input("Quantidade de Threads (Padrão 12): ", 12, 1);

    // Configura as threads
    pthread_t threads[num_processes];
    ThreadArgs thread_args[num_processes];

    for (int i = 0; i < num_processes; i++) {
        strcpy(thread_args[i].partial_key, partial_key);
        thread_args[i].thread_id = i;
        thread_args[i].refresh_time = refresh_time;

        pthread_create(&threads[i], NULL, bruteforce_worker, &thread_args[i]);
    }

    // Aguarda todas as threads finalizarem
    for (int i = 0; i < num_processes; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&file_lock);

    printf("Processo concluído.\n");
    return 0;
}
